// 303MPC engine: the Lemus TB-303 core (vendor/lemus/tb303/core, AGPL-3.0, by lemus-dev) behind the
// mpc-vst-plugins mpc_engine() contract. The original uPD650C-133 firmware needs the user's own ROM
// (d650c-133.ic8) in <plugin folder>/roms; without it the voice still plays from MIDI (voice mode).
// L'Cronx 2026. Parameter order = VST index: never reorder, only append (params.lock).
extern "C" {
#include "engine.h"
}
#include "spec_keys.h"
#include "views.h"
#include "romfind.h"
#include "machine.h"
#include "midi.h"
#include "tempoclock.h"
#include "trims.h"
#include "voice.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace {

using tb303::Machine;

constexpr float kPowerOnFrom = 0.005f;
constexpr float kTopDefaultHz = 2300; // Lemus's default FULL CUT OFF

// The pot law that gives the default full cutoff (same maths as Lemus's plugin).
const std::vector<std::pair<double, double>> &topLaw()
{
	static const auto law = [] {
		auto c = tb303::unitCalibration();
		std::vector<double> mids{c.taperAMid};
		for (double m = 0.6; m > 0.003; m *= 0.94)
			if (std::abs(m / c.taperAMid - 1) > 0.01)
				mids.push_back(m);
		std::sort(mids.begin(), mids.end(), std::greater<>());
		std::vector<std::pair<double, double>> pts;
		for (double m : mids) {
			c.cutoffTaperMid = m;
			pts.push_back({std::log(tb303::fullCutOffHz(c)), std::log(m)});
		}
		return pts;
	}();
	return law;
}

double potMidForTop(double hz)
{
	const auto &law = topLaw();
	const double x = std::log(hz);
	if (x <= law.front().first)
		return std::exp(law.front().second);
	for (size_t i = 1; i < law.size(); ++i)
		if (x <= law[i].first) {
			const double t = (x - law[i - 1].first) / (law[i].first - law[i - 1].first);
			return std::exp(law[i - 1].second + t * (law[i].second - law[i - 1].second));
		}
	return std::exp(law.back().second);
}

double tempoBpm(double knob, double rScale = 1)
{
	constexpr double rFixed = 1e6 * 40.0 / 260.0;
	constexpr double b = 32.11;
	const double x = 1 - std::clamp(knob, 0.0, 1.0);
	const double rPot = 1e6 * (std::pow(b, x) - 1) / (b - 1);
	return 300.0 * rFixed / (rFixed * rScale + rPot);
}

// Panel keys: parameter index -> the 303's matrix key.
const std::pair<int, Machine::Key> kKeyMap[] = {
	{P_K_C, Machine::Key::C}, {P_K_CS, Machine::Key::Cs}, {P_K_D, Machine::Key::D}, {P_K_DS, Machine::Key::Ds},
	{P_K_E, Machine::Key::E}, {P_K_F, Machine::Key::F}, {P_K_FS, Machine::Key::Fs}, {P_K_G, Machine::Key::G},
	{P_K_GS, Machine::Key::Gs}, {P_K_A, Machine::Key::A}, {P_K_AS, Machine::Key::As}, {P_K_B, Machine::Key::B},
	{P_K_C2, Machine::Key::C2}, {P_K_DOWN, Machine::Key::Down}, {P_K_UP, Machine::Key::Up},
	{P_K_ACCENT, Machine::Key::Accent}, {P_K_SLIDE, Machine::Key::Slide}, {P_K_CLEAR, Machine::Key::PatternClear},
	{P_K_FUNCTION, Machine::Key::Function}, {P_K_PITCH, Machine::Key::PitchMode}, {P_K_TIME, Machine::Key::TimeMode},
	{P_K_BACK, Machine::Key::Back}, {P_K_TAP, Machine::Key::Tap}};

constexpr int kKeyHoldSamples = 4410; // a panel key stays down 100 ms: the firmware scans and debounces

struct Inst {
	Machine machine;
	tb303::Voice voice;
	tb303::MidiDriver midi;
	tb303::TempoClock clock;
	std::string dataDir;
	bool romOk = false;
	double rate = 44100;
	double v[P_COUNT] = {};
	int hold[int(Machine::Key::Count)] = {};
	bool internalRun = false, powered = false, midiActive = false, linesDirty = true;
	bool voiceGate = false;
	char status[96] = "";
	std::string info;

	Inst()
	{
		for (int i = 0; i < P_COUNT; ++i)
			v[i] = kDef[i];
		auto c = tb303::unitCalibration();
		c.cutoffTaperMid = potMidForTop(kTopDefaultHz);
		voice.setCalibration(c);
		machine.setIrqSeconds(tb303::trims::irqSeconds(tb303::trims::Trims().tm2));
		voice.prepare(rate);
		machine.clearPatterns();
		powered = v[P_VOLUME] / 10 >= kPowerOnFrom;
		machine.powerOn();
	}

	void loadRom()
	{
		std::vector<uint8_t> rom;
		romOk = !dataDir.empty() &&
		        emu::findRom(dataDir, "7a4236b2bc9a5cd4c80c63ca1a193e03becfcb4c", 2048, rom) &&
		        machine.setRom(rom.data(), rom.size());
		if (romOk) {
			machine.clearPatterns();
			machine.powerOn();
		}
	}

	tb303::Controls controls() const
	{
		tb303::Controls c;
		c.tuning = v[P_TUNING] / 10;
		c.cutoff = v[P_CUTOFF] / 10;
		c.resonance = v[P_RESONANCE] / 10;
		c.envMod = v[P_ENVMOD] / 10;
		c.decay = v[P_DECAY] / 10;
		c.accent = v[P_ACCENT] / 10;
		c.volume = v[P_VOLUME] / 10;
		c.square = v[P_WAVEFORM] > 0.5;
		return c;
	}

	void doMidi(const uint8_t *m, int len)
	{
		if (len < 2)
			return;
		const int st = m[0] & 0xf0, d1 = m[1], d2 = len > 2 ? m[2] : 0;
		if (st == 0x90 && d2 > 0)
			midi.noteOn(d1, d2);
		else if (st == 0x80 || (st == 0x90 && d2 == 0))
			midi.noteOff(d1);
		else if (st == 0xB0) {
			if (d1 == 123 || d1 == 120)
				midi.allNotesOff();
			else if (d1 == 64)
				midi.setSustain(d2 >= 64);
			else {
				int idx = -1;
				switch (d1) {
				case 74: idx = P_CUTOFF; break;
				case 71: idx = P_RESONANCE; break;
				case 12: idx = P_ENVMOD; break;
				case 75: case 13: idx = P_DECAY; break;
				case 16: idx = P_ACCENT; break;
				case 104: idx = P_TUNING; break;
				case 11: idx = P_VOLUME; break;
				default: break;
				}
				if (idx >= 0)
					v[idx] = d2 / 127.0 * 10;
			}
		}
		linesDirty = true;
	}

	void renderMono(float *out, int n)
	{
		voice.setControls(controls());
		const bool power = v[P_VOLUME] / 10 >= kPowerOnFrom;
		if (power != powered) {
			powered = power;
			if (power) {
				machine.powerOn();
			} else {
				internalRun = false;
				midi.allNotesOff();
				midiActive = false;
				auto lines = machine.lines();
				lines.gate = lines.accent = lines.slide = false;
				voice.setLines(lines, machine.strobes());
				voiceGate = false;
			}
		}
		if (!power) {
			voice.render(out, n);
			return;
		}
		machine.setMode(Machine::Mode(std::clamp(int(v[P_MODE] + 0.5), 0, 3)));
		machine.setTrack(std::clamp(int(v[P_TRACK] + 0.5), 0, 6));
		const double knobBpm = tempoBpm(v[P_TEMPO] / 10, tb303::trims::tempoRScale(tb303::trims::Trims().tm1));
		const bool run = internalRun;
		int done = 0;
		const bool haveRom = machine.romLoaded();
		for (int i = 0; i < n; ++i) {
			for (int k = 0; k < int(Machine::Key::Count); ++k)
				if (hold[k] > 0 && --hold[k] == 0)
					machine.setKey(Machine::Key(k), false);
			if (haveRom) {
				const bool clk = clock.advance(1.0 / rate, knobBpm, internalRun);
				machine.setRun(run);
				machine.setClock(clk);
				if (machine.tick(rate))
					linesDirty = true;
			}
			if (linesDirty) {
				if (i > done) {
					voice.render(out + done, i - done);
					done = i;
				}
				const bool midiHeld = midi.lines().gate;
				if (midiHeld)
					midiActive = true;
				else if (haveRom && machine.lines().gate)
					midiActive = false;
				const bool useMidi = midiActive || !haveRom;
				const auto &lines = useMidi ? midi.lines() : machine.lines();
				voice.setLines(lines, useMidi ? midi.triggers() : machine.strobes());
				voiceGate = lines.gate;
				linesDirty = false;
			}
		}
		if (n > done)
			voice.render(out + done, n - done);
	}

	void pressKey(Machine::Key k)
	{
		if (!machine.romLoaded())
			return;
		machine.setKey(k, true);
		hold[int(k)] = kKeyHoldSamples;
	}

	void updateStatus()
	{
		static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C'"};
		std::string s = machine.romLoaded() ? (internalRun ? "RUN" : "STOP") : "VOICE MODE (no ROM)";
		if (machine.romLoaded()) {
			for (int i = 0; i < 13; ++i)
				if (machine.led(Machine::Led(i)) > 0.5f)
					s += std::string(" ") + names[i];
			if (machine.led(Machine::Led::Accent) > 0.5f) s += " ACC";
			if (machine.led(Machine::Led::Slide) > 0.5f) s += " SLD";
			if (machine.led(Machine::Led::Down) > 0.5f) s += " DN";
			if (machine.led(Machine::Led::Up) > 0.5f) s += " UP";
		}
		snprintf(status, sizeof status, "%s", s.c_str());
	}
};

void *create(const char *dataDir)
{
	auto *p = new Inst();
	if (dataDir)
		p->dataDir = dataDir;
	p->loadRom();
	p->updateStatus();
	return p;
}

void destroy(void *i) { delete static_cast<Inst *>(i); }

void midi(void *i, const uint8_t *m, int len) { static_cast<Inst *>(i)->doMidi(m, len); }

int keyIndex(const char *key)
{
	for (int i = 0; i < P_COUNT; ++i)
		if (!strcmp(key, kKeys[i]))
			return i;
	return -1;
}

void setIdx(Inst *p, int idx, double x)
{
	if (emu::viewSet(p->v, idx, x))
		return;
	if (idx != P_STATUS && idx != P_INFO && idx != P_BRAND && !(idx >= P_QL1 && idx < P_QL1 + 16)) {
		bool pressed = idx == P_RUN;
		for (const auto &km : kKeyMap) pressed = pressed || km.first == idx;
		if (pressed) { if (x > 0.5) p->info = kNames[idx]; }
		else p->info = std::string(kNames[idx]) + "  " + emu::valueText(idx, x);
	}
	if (idx == P_RUN) {
		if (x > 0.5 && p->machine.romLoaded())
			p->internalRun = !p->internalRun;
		return;
	}
	for (const auto &km : kKeyMap)
		if (km.first == idx) {
			if (x > 0.5)
				p->pressKey(km.second);
			return;
		}
	if (idx == P_STATUS || idx == P_INFO || idx == P_BRAND)
		return;
	p->v[idx] = std::clamp(x, kMin[idx], kMax[idx]);
}

void set_param(void *inst, const char *key, const char *val)
{
	auto *p = static_cast<Inst *>(inst);
	if (!strcmp(key, "state")) {
		// pattern RAM, one hex digit per nibble
		std::vector<uint8_t> ram;
		for (const char *c = val; *c; ++c) {
			const char ch = *c;
			if (ch >= '0' && ch <= '9') ram.push_back(uint8_t(ch - '0'));
			else if (ch >= 'a' && ch <= 'f') ram.push_back(uint8_t(ch - 'a' + 10));
		}
		if (!ram.empty() && p->machine.romLoaded())
			p->machine.setPatternRam(ram.data(), ram.size());
		return;
	}
	const int idx = keyIndex(key);
	if (idx < 0)
		return;
	const double x = atof(val);
	if (idx >= P_QL1 && idx < P_QL1 + 16) {
		const int t = emu::qlTarget(p->v, idx - P_QL1);
		if (t >= 0)
			setIdx(p, t, kMin[t] + x * (kMax[t] - kMin[t]));
		return;
	}
	setIdx(p, idx, x);
}

int out(char *buf, int len, const std::string &s) { return snprintf(buf, size_t(len), "%s", s.c_str()); }

int get_param(void *inst, const char *key, char *buf, int len)
{
	auto *p = static_cast<Inst *>(inst);
	if (!strcmp(key, "state")) {
		const auto &ram = p->machine.patternRam();
		std::string s;
		s.reserve(ram.size());
		for (uint8_t n : ram)
			s += "0123456789abcdef"[n & 0xf];
		if (int(s.size()) + 1 > len)
			return 0;
		memcpy(buf, s.c_str(), s.size() + 1);
		return int(s.size()) + 1;
	}
	std::string k = key, sfx;
	if (k.size() > 5 && k.compare(k.size() - 5, 5, "_name") == 0) { sfx = "name"; k.resize(k.size() - 5); }
	else if (k.size() > 8 && k.compare(k.size() - 8, 8, "_display") == 0) { sfx = "display"; k.resize(k.size() - 8); }
	const int idx = keyIndex(k.c_str());
	if (idx < 0)
		return 0;
	if (idx >= P_QL1 && idx < P_QL1 + 16) {
		const int t = emu::qlTarget(p->v, idx - P_QL1);
		if (sfx == "name")
			return out(buf, len, t >= 0 ? kNames[t] : "-");
		if (t < 0)
			return out(buf, len, sfx == "display" ? "" : "0");
		if (sfx == "display")
			return out(buf, len, emu::valueText(t, p->v[t]));
		return snprintf(buf, size_t(len), "%g", (p->v[t] - kMin[t]) / (kMax[t] - kMin[t]));
	}
	if (!sfx.empty())
		return 0;
	if (idx == P_STATUS) {
		p->updateStatus();
		return out(buf, len, p->status);
	}
	if (idx == P_INFO)
		return out(buf, len, p->info.empty() ? "TOUCH A CONTROL" : p->info);
	if (idx == P_BRAND)
		return out(buf, len, "L'Cronx");
	if (idx == P_RUN)
		return snprintf(buf, size_t(len), "%d", p->internalRun ? 1 : 0);
	return snprintf(buf, size_t(len), "%g", p->v[idx]);
}

void render(void *inst, int16_t *out_lr, int frames)
{
	auto *p = static_cast<Inst *>(inst);
	static thread_local std::vector<float> mono;
	mono.assign(size_t(frames), 0.0f);
	p->renderMono(mono.data(), frames);
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
