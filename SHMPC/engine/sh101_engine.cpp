// SHMPC engine: the Lemus SH-101 core (vendor/lemus/sh101/core, AGPL-3.0, by lemus-dev) behind the mpc-vst-plugins mpc_engine()
// contract. The processing follows Lemus's own plugin (machines/sh101/plugin/PluginProcessor.cpp): the original TMP80C49 firmware
// on the MCS-48 core drives the synth-board model. The firmware needs the user's own ROM (tmp80c49p-6-7301.ic6) in
// <plugin folder>/roms; without it MIDI plays the synth board directly (voice mode: no sequencer or arpeggio).
// Not in this build: POWER, host sync (the MODULATOR clocks the arpeggio/sequencer), the bender lever (use pitch bend / mod wheel).
// L'Cronx 2026. Parameter order = VST index: never reorder, only append (params.lock).
extern "C" {
#include "engine.h"
}
#include "spec_keys.h"
#include "views.h"
#include "romfind.h"
#include "lfo.h"
#include "machine.h"
#include "midi.h"
#include "voice.h"
#include "voice_keys.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace {

using sh101::Machine;

constexpr int kHoldSamples = 4410;  // a panel button stays down 100 ms: the firmware latches once per main loop
constexpr int kNoteSamples = 11025; // a touched key sounds for 250 ms

const std::pair<int, Machine::Button> kButtons[] = {
	{P_B_LOAD, Machine::Button::Load}, {P_B_PLAY, Machine::Button::Play}, {P_B_DOWN, Machine::Button::ArpDown},
	{P_B_UD, Machine::Button::ArpUpDown}, {P_B_UP, Machine::Button::ArpUp}, {P_B_HOLD, Machine::Button::Hold},
	{P_B_KEYTR, Machine::Button::KeyTranspose}};

// MIDI CC -> parameter (sliders/knobs 0..10, choices by index), after Lemus's table
struct CcMap { int cc, idx; };
const CcMap kCc[] = {
	{3, P_LFORATE}, {5, P_PORTAMENTO}, {12, P_LFOWAVE}, {13, P_VCOMOD}, {14, P_RANGE}, {15, P_PULSEWIDTH}, {16, P_PWM},
	{17, P_LFOMOD}, {18, P_BENDVCO}, {19, P_PULSELEVEL}, {20, P_SAWLEVEL}, {21, P_SUBLEVEL}, {22, P_SUBMODE},
	{23, P_NOISELEVEL}, {24, P_VCFENV}, {25, P_VCFMOD}, {26, P_VCFKYBD}, {27, P_BENDVCF}, {28, P_VCA}, {29, P_TRIGGER},
	{30, P_SUSTAIN}, {71, P_VCFRES}, {72, P_RELEASE}, {73, P_ATTACK}, {74, P_VCFFREQ}, {75, P_DECAY}, {76, P_TUNE}};

struct Inst {
	Machine machine;
	sh101::Voice voice;
	sh101::Lfo lfo;
	sh101::KeyboardDriver keyboard;
	sh101::VoiceKeys voiceKeys;
	std::string dataDir;
	bool romOk = false;
	double rate = 44100;
	double v[P_COUNT] = {};
	int portaMode = 2, transpose = 1; // OFF, M
	double midiBend = 0, midiMod = 0;
	std::vector<sh101::ModFrame> mod;
	struct Ev { int key; bool down; };
	std::vector<Ev> queue;
	uint64_t changed[Machine::kKeys + int(Machine::Button::Count)] = {};
	int release[int(Machine::Button::Count)] = {};
	struct NoteOff { int note, in; };
	std::vector<NoteOff> noteOffs;
	bool asleep = false;
	int quietSamples = 0;
	sh101::Voice::Probe quietFrom;
	sh101::CpuLines voiceLines;
	sh101::Controls lastControls;
	sh101::Performance lastPerf;
	char status[96] = "";
	std::string info;

	Inst()
	{
		for (int i = 0; i < P_COUNT; ++i)
			v[i] = kDef[i];
		voice.setCalibration(sh101::Calibration::realUnit());
		voice.setControls(controls());
		voice.prepare(rate);
		lfo.prepare(rate);
		voiceKeys.prepare(rate);
		mod.assign(128, {});
		machine.writeSequence({});
		machine.powerOn();
		voice.settlePower();
	}

	void loadRom()
	{
		std::vector<uint8_t> rom;
		romOk = !dataDir.empty() && emu::findRom(dataDir, "a84bb1758582dbb800001db3663eaf3fb9d0e25d", 2048, rom) &&
		        machine.setRom(rom.data(), rom.size());
		if (romOk) {
			machine.writeSequence({});
			machine.powerOn();
		}
	}

	void wake() { asleep = false; quietSamples = 0; }
	int ch(int idx) const { return std::clamp(int(v[idx] + 0.5), int(kMin[idx]), int(kMax[idx])); }

	sh101::Controls controls() const
	{
		sh101::Controls c;
		c.lfoRate = v[P_LFORATE] / 10;
		c.lfoWave = sh101::LfoWave(ch(P_LFOWAVE));
		c.vcoMod = v[P_VCOMOD] / 10;
		c.pulseWidth = v[P_PULSEWIDTH] / 10;
		c.pwmSource = sh101::PwmSource(ch(P_PWM));
		c.pulseLevel = v[P_PULSELEVEL] / 10;
		c.sawLevel = v[P_SAWLEVEL] / 10;
		c.subLevel = v[P_SUBLEVEL] / 10;
		c.noiseLevel = v[P_NOISELEVEL] / 10;
		c.subMode = sh101::SubMode(ch(P_SUBMODE));
		c.vcfFreq = v[P_VCFFREQ] / 10;
		c.vcfRes = v[P_VCFRES] / 10;
		c.vcfEnv = v[P_VCFENV] / 10;
		c.vcfMod = v[P_VCFMOD] / 10;
		c.vcfKybd = v[P_VCFKYBD] / 10;
		c.vcaMode = sh101::VcaMode(ch(P_VCA));
		c.attack = v[P_ATTACK] / 10;
		c.decay = v[P_DECAY] / 10;
		c.sustain = v[P_SUSTAIN] / 10;
		c.release = v[P_RELEASE] / 10;
		c.trigger = sh101::Trigger(ch(P_TRIGGER));
		c.volume = v[P_VOLUME] / 10;
		c.tune = v[P_TUNE] / 10;
		c.portamento = v[P_PORTAMENTO] / 10;
		c.portaMode = sh101::PortaMode(portaMode);
		c.bendVco = v[P_BENDVCO] / 10;
		c.bendVcf = v[P_BENDVCF] / 10;
		c.lfoModDepth = v[P_LFOMOD] / 10;
		return c;
	}
	static bool same(const sh101::Controls &a, const sh101::Controls &b)
	{
		return a.lfoRate == b.lfoRate && a.lfoWave == b.lfoWave && a.vcoMod == b.vcoMod && a.pulseWidth == b.pulseWidth &&
		       a.pwmSource == b.pwmSource && a.pulseLevel == b.pulseLevel && a.sawLevel == b.sawLevel && a.subLevel == b.subLevel &&
		       a.noiseLevel == b.noiseLevel && a.subMode == b.subMode && a.vcfFreq == b.vcfFreq && a.vcfRes == b.vcfRes &&
		       a.vcfEnv == b.vcfEnv && a.vcfMod == b.vcfMod && a.vcfKybd == b.vcfKybd && a.vcaMode == b.vcaMode &&
		       a.attack == b.attack && a.decay == b.decay && a.sustain == b.sustain && a.release == b.release &&
		       a.trigger == b.trigger && a.volume == b.volume && a.tune == b.tune && a.portamento == b.portamento &&
		       a.portaMode == b.portaMode && a.bendVco == b.bendVco && a.bendVcf == b.bendVcf && a.lfoModDepth == b.lfoModDepth;
	}

	void playNote(int note, bool on)
	{
		if (on) {
			if (romOk) keyboard.noteOn(note);
			else voiceKeys.noteOn(note, sh101::Trigger(ch(P_TRIGGER)));
		} else {
			keyboard.noteOff(note);
			voiceKeys.noteOff(note);
		}
	}

	void doMidi(const uint8_t *m, int len)
	{
		if (len < 2) return;
		const int st = m[0] & 0xf0, d1 = m[1], d2 = len > 2 ? m[2] : 0;
		if (st == 0x90 && d2 > 0) playNote(d1, true);
		else if (st == 0x80 || (st == 0x90 && d2 == 0)) playNote(d1, false);
		else if (st == 0xE0) midiBend = std::clamp(((d2 << 7 | d1) - 8192) / 8191.0, -1.0, 1.0);
		else if (st == 0xB0) {
			if (d1 == 123 || d1 == 120) { keyboard.allNotesOff(); voiceKeys.allNotesOff(); }
			else if (d1 == 1) midiMod = d2 / 127.0;
			else if (d1 == 64) { machine.setHoldPedal(d2 >= 64); voiceKeys.setPedal(d2 >= 64); }
			else
				for (const auto &e : kCc)
					if (e.cc == d1) {
						v[e.idx] = kOpts[e.idx][0] ? std::min<double>(kMax[e.idx], int(d2 / 128.0 * (kMax[e.idx] + 1))) : d2 / 127.0 * 10;
						if (!kOpts[e.idx][0] && kMax[e.idx] < 10) v[e.idx] = std::min<double>(kMax[e.idx], int(d2 / 128.0 * (kMax[e.idx] + 1)));
					}
		}
	}

	void renderVoice(float *out, int n, const sh101::ModFrame *md)
	{
		if (asleep) { std::fill(out, out + n, 0.0f); return; }
		voice.render(out, n, md);
		float mx = 0;
		for (int i = 0; i < n; ++i) mx = std::max(mx, std::abs(out[i]));
		if (voiceLines.gate || voice.powering() || voice.poweringDown() || mx > 1e-6f) { quietSamples = 0; return; }
		const auto p = voice.probe();
		if (quietSamples == 0) quietFrom = p;
		quietSamples += n;
		if (quietSamples < int(0.1 * rate)) return;
		if (std::abs(p.env - quietFrom.env) > 1e-6 || std::abs(p.vca - quietFrom.vca) > 1e-8 || std::abs(p.cvVolts - quietFrom.cvVolts) > 1e-5) {
			quietFrom = p; quietSamples = n; return;
		}
		asleep = true;
	}

	void block(float *out, int n)
	{
		const auto c = controls();
		if (!same(c, lastControls)) { lastControls = c; wake(); }
		voice.setControls(c);
		lfo.setRate(c.lfoRate);
		lfo.setWave(c.lfoWave);
		machine.setRange(sh101::Range(ch(P_RANGE)));
		machine.setTrigger(c.trigger);
		const auto panelTranspose = sh101::Transpose(transpose);
		// timed releases (buttons, touched keys)
		for (int b = 0; b < int(Machine::Button::Count); ++b)
			if (release[b] > 0 && (release[b] -= n) <= 0) { release[b] = 0; queue.push_back({-1 - b, false}); }
		for (auto &no : noteOffs) no.in -= n;
		for (size_t i = 0; i < noteOffs.size();)
			if (noteOffs[i].in <= 0) { playNote(noteOffs[i].note, false); noteOffs.erase(noteOffs.begin() + long(i)); }
			else ++i;
		if (!romOk) queue.clear();
		{
			const uint64_t loops = machine.loops();
			size_t applied = 0;
			for (; applied < queue.size(); ++applied) {
				const auto &e = queue[applied];
				auto &since = changed[size_t(e.key >= 0 ? e.key : Machine::kKeys - 1 - e.key)];
				if (since != 0 && loops < since + 2) break;
				since = loops == 0 ? 1 : loops;
				if (e.key >= 0) machine.setKey(e.key, e.down);
				else machine.setButton(Machine::Button(-1 - e.key), e.down);
			}
			queue.erase(queue.begin(), queue.begin() + long(applied));
		}
		sh101::Performance perf;
		perf.bend = midiBend;
		perf.modPush = midiMod;
		if (perf.bend != lastPerf.bend || perf.modPush != lastPerf.modPush) { lastPerf = perf; wake(); }
		voice.setPerformance(perf);
		if (int(mod.size()) < n) mod.resize(size_t(n));
		int done = 0;
		for (int i = 0; i < n; ++i) {
			if (romOk) keyboard.tick(machine, panelTranspose);
			const auto l = lfo.tick();
			mod[size_t(i)] = {l.tri, l.square};
			machine.setClock(l.square);
			if (romOk ? machine.tick(rate) : voiceKeys.tick(sh101::Range(ch(P_RANGE)), c.trigger, panelTranspose)) {
				if (i > done) { renderVoice(out + done, i - done, mod.data() + done); done = i; }
				const auto &lines = romOk ? machine.lines() : voiceKeys.lines();
				if (lines.gate != voiceLines.gate || lines.kcv != voiceLines.kcv || lines.range != voiceLines.range ||
				    lines.portaOff != voiceLines.portaOff)
					wake();
				voiceLines = lines;
				voice.setLines(lines);
				lfo.setClkRst(lines.clkRst);
			}
		}
		if (n > done) renderVoice(out + done, n - done, mod.data() + done);
	}

	void updateStatus()
	{
		static const char *led[] = {"LOAD", "PLAY", "DOWN", "U/D", "UP", "HOLD", "KEY TR"};
		std::string s = romOk ? "ROM" : "VOICE MODE (no ROM)";
		if (romOk)
			for (int i = 0; i < int(Machine::Led::Count); ++i)
				if (machine.led(Machine::Led(i)) > 0.5f) s += std::string(" ") + led[i];
		snprintf(status, sizeof status, "%s", s.c_str());
	}
};

void *create(const char *dataDir)
{
	auto *p = new Inst();
	if (dataDir) p->dataDir = dataDir;
	p->loadRom();
	p->updateStatus();
	return p;
}
void destroy(void *i) { delete static_cast<Inst *>(i); }
void midi(void *i, const uint8_t *m, int len) { static_cast<Inst *>(i)->doMidi(m, len); }

int keyIndex(const char *key)
{
	for (int i = 0; i < P_COUNT; ++i)
		if (!strcmp(key, kKeys[i])) return i;
	return -1;
}

void setIdx(Inst *p, int idx, double x)
{
	if (idx >= P_KEY_0 && idx < P_KEY_0 + sh101::Machine::kKeys) {
		if (x > 0.5) {
			p->info = kNames[idx];
			const int note = sh101::KeyboardDriver::kLowestNote + (idx - P_KEY_0);
			p->playNote(note, true);
			p->noteOffs.push_back({note, kNoteSamples});
		}
		return;
	}
	for (const auto &b : kButtons)
		if (b.first == idx) {
			if (x > 0.5 && p->romOk) {
				p->info = kNames[idx];
				p->queue.push_back({-1 - int(b.second), true});
				p->release[int(b.second)] = kHoldSamples;
			}
			return;
		}
	if (idx == P_PORTAMODE || idx == P_TRANSPOSE) {
		if (x > 0.5) {
			static const char *pm[] = {"ON", "AUTO", "OFF"}, *tr[] = {"L", "M", "H"};
			int &s = idx == P_PORTAMODE ? p->portaMode : p->transpose;
			s = (s + 1) % 3;
			p->info = std::string(kNames[idx]) + "  " + (idx == P_PORTAMODE ? pm[s] : tr[s]);
			p->wake();
		}
		return;
	}
	if (idx == P_STATUS || idx == P_INFO || idx == P_BRAND) return;
	p->v[idx] = std::clamp(x, kMin[idx], kMax[idx]);
	p->info = std::string(kNames[idx]) + "  " + emu::valueText(idx, p->v[idx]);
}

void set_param(void *inst, const char *key, const char *val)
{
	auto *p = static_cast<Inst *>(inst);
	if (!strcmp(key, "state")) {
		std::vector<uint8_t> b;
		int hi = -1;
		for (const char *c = val; *c; ++c) {
			int d = (*c >= '0' && *c <= '9') ? *c - '0' : (*c >= 'a' && *c <= 'f') ? *c - 'a' + 10 : -1;
			if (d < 0) continue;
			if (hi < 0) hi = d; else { b.push_back(uint8_t(hi << 4 | d)); hi = -1; }
		}
		if (!b.empty()) { p->machine.loadState(b); p->machine.powerOn(); }
		return;
	}
	const int idx = keyIndex(key);
	if (idx < 0) return;
	const double x = atof(val);
	if (idx >= P_QL1 && idx < P_QL1 + 16) {
		const int t = emu::qlTarget(p->v, idx - P_QL1);
		if (t >= 0) setIdx(p, t, kMin[t] + x * (kMax[t] - kMin[t]));
		return;
	}
	setIdx(p, idx, x);
}

int out(char *buf, int len, const std::string &s) { return snprintf(buf, size_t(len), "%s", s.c_str()); }

int get_param(void *inst, const char *key, char *buf, int len)
{
	auto *p = static_cast<Inst *>(inst);
	if (!strcmp(key, "state")) {
		const auto st = p->machine.saveState();
		std::string s;
		for (uint8_t b : st) { s += "0123456789abcdef"[b >> 4]; s += "0123456789abcdef"[b & 15]; }
		if (int(s.size()) + 1 > len) return 0;
		memcpy(buf, s.c_str(), s.size() + 1);
		return int(s.size()) + 1;
	}
	std::string k = key, sfx;
	if (k.size() > 5 && k.compare(k.size() - 5, 5, "_name") == 0) { sfx = "name"; k.resize(k.size() - 5); }
	else if (k.size() > 8 && k.compare(k.size() - 8, 8, "_display") == 0) { sfx = "display"; k.resize(k.size() - 8); }
	const int idx = keyIndex(k.c_str());
	if (idx < 0) return 0;
	if (idx >= P_QL1 && idx < P_QL1 + 16) {
		const int t = emu::qlTarget(p->v, idx - P_QL1);
		if (sfx == "name") return out(buf, len, t >= 0 ? kNames[t] : "-");
		if (t < 0) return out(buf, len, sfx == "display" ? "" : "0");
		if (sfx == "display") return out(buf, len, emu::valueText(t, p->v[t]));
		return snprintf(buf, size_t(len), "%g", (p->v[t] - kMin[t]) / (kMax[t] - kMin[t]));
	}
	if (!sfx.empty()) return 0;
	if (idx == P_STATUS) { p->updateStatus(); return out(buf, len, p->status); }
	if (idx == P_INFO) return out(buf, len, p->info.empty() ? "TOUCH A CONTROL" : p->info);
	if (idx == P_BRAND) return out(buf, len, "L'Cronx");
	return snprintf(buf, size_t(len), "%g", p->v[idx]);
}

void render(void *inst, int16_t *out_lr, int frames)
{
	auto *p = static_cast<Inst *>(inst);
	static thread_local std::vector<float> mono;
	mono.assign(size_t(frames), 0.0f);
	p->block(mono.data(), frames);
	for (int i = 0; i < frames; ++i) {
		const float s = std::clamp(mono[size_t(i)], -1.0f, 1.0f);
		const int16_t q = int16_t(std::lrintf(s * 32767.0f));
		out_lr[2 * i] = q;
		out_lr[2 * i + 1] = q;
	}
}

const mpc_engine_t kEngine = {create, destroy, midi, set_param, get_param, render, nullptr, nullptr};

} // namespace

extern "C" const mpc_engine_t *mpc_engine(void) { return &kEngine; }
