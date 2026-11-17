// 808MPC engine: the Lemus TR-808 core (vendor/lemus/tr808/core, AGPL-3.0, by lemus-dev) behind the mpc-vst-plugins mpc_engine()
// contract. The sequencer is the original uPD650C-085 firmware on an emulated uCOM-43, the voices are the voicing board solved from
// its netlists. The sequencer needs the user's own ROM (d650c-085.ic4) in <plugin folder>/roms, found by SHA-1; without it the
// voices play from MIDI (voice mode).
// Not measured on a Force: the board is a circuit solver and may not run in real time on MPC OS (see docs/NOTES.md).
// L'Cronx 2026. Parameter order = VST index: never reorder, only append (params.lock).
extern "C" {
#include "engine.h"
}
#include "spec_keys.h"
#include "views.h"
#include "romfind.h"
#include "tr808/core/machine.h"
#include "voiceboard.h"
#include "parallel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

using tr808::Machine;

constexpr int kKeyHoldSamples = 5292; // a panel key stays down 120 ms
constexpr int kAccentVelocity = 100;  // a MIDI note at or above this velocity is an accented step

struct KnobMap { int p; double tr808::Controls::*field; };
const KnobMap kKnobs[] = {
	{P_AC_LEVEL, &tr808::Controls::accent}, {P_BD_LEVEL, &tr808::Controls::bdLevel}, {P_BD_TONE, &tr808::Controls::bdTone},
	{P_BD_DECAY, &tr808::Controls::bdDecay}, {P_SD_LEVEL, &tr808::Controls::sdLevel}, {P_SD_TONE, &tr808::Controls::sdTone},
	{P_SD_SNAPPY, &tr808::Controls::sdSnappy}, {P_LT_LEVEL, &tr808::Controls::ltLevel}, {P_LT_TUNING, &tr808::Controls::ltTuning},
	{P_MT_LEVEL, &tr808::Controls::mtLevel}, {P_MT_TUNING, &tr808::Controls::mtTuning}, {P_HT_LEVEL, &tr808::Controls::htLevel},
	{P_HT_TUNING, &tr808::Controls::htTuning}, {P_RS_LEVEL, &tr808::Controls::rsLevel}, {P_CP_LEVEL, &tr808::Controls::cpLevel},
	{P_CB_LEVEL, &tr808::Controls::cbLevel}, {P_CY_LEVEL, &tr808::Controls::cyLevel}, {P_CY_TONE, &tr808::Controls::cyTone},
	{P_CY_DECAY, &tr808::Controls::cyDecay}, {P_OH_LEVEL, &tr808::Controls::ohLevel}, {P_OH_DECAY, &tr808::Controls::ohDecay},
	{P_CH_LEVEL, &tr808::Controls::chLevel}, {P_VOLUME, &tr808::Controls::volume},
};

uint16_t noteVoice(int note)
{
	using namespace tr808;
	switch (note) {
	case 35: case 36: return kBitBD;
	case 37: case 75: return kBitRS;
	case 38: case 40: return kBitSD;
	case 39: case 70: return kBitCP;
	case 41: case 43: case 64: return kBitLT;
	case 45: case 47: case 63: return kBitMT;
	case 48: case 50: case 62: return kBitHT;
	case 42: case 44: return kBitCH;
	case 46: return kBitOH;
	case 49: case 57: return kBitCY;
	case 56: return kBitCB;
	default: return 0;
	}
}

const char *const kVariationNames[3] = {"A", "A+B", "B"};
const char *const kPrescaleNames[4] = {"1", "2", "3", "4"};

struct Inst {
	Machine machine;
	tr808::VoiceBoard board;
	tr808::TempoClock clock;
	std::unique_ptr<tr808::Parallel> parallel;
	std::string dataDir;
	bool romOk = false;
	double rate = 44100;
	double v[P_COUNT] = {};
	int hold[18] = {}; // 0-15 the step keys, 16 clear, 17 tap
	bool internalRun = false, asleep = false, tapDown = false, pulseLevel = false;
	int variation = 0, prescale = 2;
	int cpuPulse = 0, midiPulse = 0;
	uint16_t midiBits = 0;
	unsigned lastPulses = 0;
	tr808::Controls lastControls;
	bool haveControls = false;
	std::string info;
	char status[96] = "";

	Inst()
	{
		for (int i = 0; i < P_COUNT; ++i)
			v[i] = kDef[i];
		// TEMPO at 120 BPM with FINE at 12 o'clock
		double lo = 0, hi = 1;
		for (int k = 0; k < 40; ++k) {
			const double mid = (lo + hi) / 2;
			(tr808::TempoClock::bpm(mid, 0.5) < 120 ? lo : hi) = mid;
		}
		v[P_TEMPO] = (lo + hi) / 2 * 10;
	}

	void init()
	{
		std::vector<uint8_t> rom;
		romOk = !dataDir.empty() && emu::findRom(dataDir, "eddac7af6396c3f220914d9cad2c9e398872b034", 2048, rom) && machine.setRom(rom.data(), rom.size());
		machine.writeFactoryRhythms(); // a new instance is a new 808; a saved session's RAM replaces it
		machine.powerOn();
		board.setCalibration(tr808::unitCalibration());
		const int cores = int(std::thread::hardware_concurrency());
		const int workers = std::clamp(cores - 2, 0, 4);
		parallel = std::make_unique<tr808::Parallel>(workers, [](int) {}, [](int) {});
		board.setParallel(workers > 0 ? parallel.get() : nullptr);
		board.setControls(controls());
		board.prepare(rate);
		lastPulses = machine.stepPulses();
	}

	tr808::Controls controls() const
	{
		tr808::Controls c;
		for (const auto &k : kKnobs)
			c.*(k.field) = v[k.p] / 10;
		c.lc = v[P_LC] > 0.5; c.mc = v[P_MC] > 0.5; c.hc = v[P_HC] > 0.5; c.cl = v[P_CL] > 0.5; c.ma = v[P_MA] > 0.5;
		return c;
	}

	void doMidi(const uint8_t *m, int len)
	{
		if (len < 1)
			return;
		const int st = m[0] & 0xf0, d1 = len > 1 ? m[1] : 0, d2 = len > 2 ? m[2] : 0;
		if (st == 0x90 && d2 > 0) {
			const uint16_t bit = noteVoice(d1);
			if (bit == 0)
				return;
			if (midiPulse <= 0 || midiPulse < int(0.001 * rate) - 1)
				midiBits = 0;
			midiBits |= bit;
			if (d2 >= kAccentVelocity)
				midiBits |= tr808::kBitAC;
			midiPulse = std::max(1, int(0.0012 * rate));
			pulseLevel = false;
		} else if (st == 0xB0) {
			// Roland TR-8's control changes for the same controls; MASTER VOLUME is CC 7
			static const std::pair<int, int> kCc[] = {
				{71, P_AC_LEVEL}, {21, P_BD_TONE}, {23, P_BD_DECAY}, {24, P_BD_LEVEL}, {25, P_SD_TONE}, {26, P_SD_SNAPPY}, {29, P_SD_LEVEL},
				{46, P_LT_TUNING}, {48, P_LT_LEVEL}, {49, P_MT_TUNING}, {51, P_MT_LEVEL}, {52, P_HT_TUNING}, {54, P_HT_LEVEL},
				{57, P_RS_LEVEL}, {60, P_CP_LEVEL}, {63, P_CH_LEVEL}, {81, P_OH_DECAY}, {82, P_OH_LEVEL}, {83, P_CY_TONE},
				{84, P_CY_DECAY}, {85, P_CY_LEVEL}, {88, P_CB_LEVEL}, {7, P_VOLUME}};
			for (const auto &c : kCc)
				if (c.first == d1)
					v[c.second] = d2 / 127.0 * 10;
		}
	}

	void pressStep(int slot)
	{
		if (!romOk)
			return;
		if (slot < 16) machine.setStepKey(slot, true);
		else if (slot == 16) machine.setClearKey(true);
		else tapDown = true;
		hold[slot] = kKeyHoldSamples;
	}

	void releaseDue(int n)
	{
		for (int s = 0; s < 18; ++s)
			if (hold[s] > 0 && (hold[s] -= n) <= 0) {
				hold[s] = 0;
				if (s < 16) machine.setStepKey(s, false);
				else if (s == 16) machine.setClearKey(false);
				else tapDown = false;
			}
	}

	void renderMono(float *out, int n)
	{
		const auto c = controls();
		if (!haveControls || std::memcmp(&c, &lastControls, sizeof c) != 0) {
			board.setControls(c);
			lastControls = c;
			haveControls = true;
			asleep = false;
		}
		using M = Machine;
		machine.setMode(M::Mode(std::clamp(int(v[P_MODE] + 0.5), 0, 5)));
		machine.setInstrument(std::clamp(int(v[P_INSTRUMENT] + 0.5), 0, 11));
		machine.setAutoFill(M::AutoFill(std::clamp(int(v[P_AUTOFILL] + 0.5), 0, 5)));
		machine.setVariation(M::Variation(variation));
		machine.setIfVariationB(v[P_IFVAR] > 0.5);
		machine.setPrescale(prescale);
		const double knobBpm = tr808::TempoClock::bpm(v[P_TEMPO] / 10, v[P_FINE] / 10);
		const bool run = internalRun;
		int done = 0;
		const int monoSamples = std::max(1, int(0.0012 * rate));
		auto renderTo = [&](int upto) {
			if (upto <= done)
				return;
			if (asleep) std::fill(out + done, out + upto, 0.0f);
			else board.render(out + done, upto - done);
			done = upto;
		};
		for (int i = 0; i < n; ++i) {
			releaseDue(1);
			const bool clk = clock.advance(1.0 / rate, knobBpm, internalRun);
			bool edge = false;
			if (romOk) {
				machine.setRun(run);
				machine.setClock(clk);
				machine.setFillIn(tapDown);
				machine.tick(rate);
				if (machine.stepPulses() != lastPulses) {
					lastPulses = machine.stepPulses();
					cpuPulse = monoSamples;
					edge = true;
				}
			}
			if (midiPulse > 0 && !pulseLevel) {
				edge = true;
				pulseLevel = true;
			}
			if (edge || cpuPulse > 0 || midiPulse > 0) {
				renderTo(i);
				const uint16_t data = uint16_t((cpuPulse > 0 ? machine.instrumentData() : 0) | (midiPulse > 0 ? midiBits : 0));
				board.setLines(edge, data);
				if (edge)
					asleep = false;
				if (cpuPulse > 0)
					--cpuPulse;
				if (midiPulse > 0 && --midiPulse == 0) {
					midiBits = 0;
					pulseLevel = false;
				}
				if (!(cpuPulse > 0 || midiPulse > 0)) {
					renderTo(i + 1);
					board.setLines(false, 0);
				}
			}
		}
		renderTo(n);
		if (!asleep && cpuPulse == 0 && midiPulse == 0 && board.asleep())
			asleep = true;
	}

	void updateStatus()
	{
		std::string s = romOk ? (internalRun ? "RUN" : "STOP") : "VOICE MODE (no program ROM)";
		if (romOk) {
			s += " STEP ";
			for (int k = 0; k < 16; ++k)
				if (machine.stepLed(k) > 0.5f) { s += std::to_string(k + 1); break; }
		}
		snprintf(status, sizeof status, "%s", s.c_str());
	}

	std::string valueText(int idx, double x) const
	{
		static const char *const mode[] = {"PATTERN CLEAR", "1st PART", "2nd PART", "MANUAL PLAY", "PLAY", "COMPOSE"};
		static const char *const inst[] = {"1 AC", "2 BD", "3 SD", "4 LT", "5 MT", "6 HT", "7 RS", "8 CP", "9 CB", "10 CY", "11 OH", "12 CH"};
		static const char *const fill[] = {"MANUAL", "16", "12", "8", "4", "2"};
		const int i = int(x + 0.5);
		if (idx == P_MODE) return mode[std::clamp(i, 0, 5)];
		if (idx == P_INSTRUMENT) return inst[std::clamp(i, 0, 11)];
		if (idx == P_AUTOFILL) return fill[std::clamp(i, 0, 5)];
		if (idx == P_TEMPO) {
			char b[32];
			snprintf(b, sizeof b, "%.1f BPM", tr808::TempoClock::bpm(x / 10, v[P_FINE] / 10));
			return b;
		}
		if (idx == P_LC || idx == P_MC || idx == P_HC) return x > 0.5 ? "CONGA" : "TOM";
		if (idx == P_CL) return x > 0.5 ? "CLAVES" : "RIM SHOT";
		if (idx == P_MA) return x > 0.5 ? "MARACAS" : "HAND CLAP";
		if (idx == P_IFVAR) return x > 0.5 ? "B" : "A";
		return emu::valueText(idx, x);
	}
};

void *create(const char *dataDir)
{
	auto *p = new Inst();
	if (dataDir)
		p->dataDir = dataDir;
	p->init();
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
	if (idx == P_STATUS || idx == P_INFO || idx == P_BRAND || (idx >= P_QL1 && idx < P_QL1 + 16))
		return;
	if (idx == P_K_STARTSTOP) {
		if (x > 0.5) {
			p->info = "START/STOP";
			if (p->romOk)
				p->internalRun = !p->internalRun;
		}
		return;
	}
	if (idx == P_K_TAP || idx == P_K_CLEAR || (idx >= P_K_STEP1 && idx < P_K_STEP1 + 16)) {
		if (x > 0.5) {
			p->info = kNames[idx];
			p->pressStep(idx == P_K_TAP ? 17 : idx == P_K_CLEAR ? 16 : idx - P_K_STEP1);
		}
		return;
	}
	if (idx == P_VARIATION || idx == P_PRESCALE) {
		if (x > 0.5) {
			int &s = idx == P_VARIATION ? p->variation : p->prescale;
			s = (s + 1) % (idx == P_VARIATION ? 3 : 4);
			p->info = std::string(kNames[idx]) + "  " + (idx == P_VARIATION ? kVariationNames[s] : kPrescaleNames[s]);
		}
		return;
	}
	p->info = std::string(kNames[idx]) + "  " + p->valueText(idx, x);
	p->v[idx] = std::clamp(x, kMin[idx], kMax[idx]);
}

// the pattern RAM (4096 nibbles) as one hex digit per nibble, runs of the same one as "h*count;" (the wrapper's state buffer is 8 KB)
std::string ramText(const std::array<uint8_t, Machine::kRamNibbles> &ram)
{
	std::string s;
	for (size_t i = 0; i < ram.size();) {
		size_t j = i;
		while (j < ram.size() && ram[j] == ram[i]) ++j;
		const char h = "0123456789abcdef"[ram[i] & 15];
		if (j - i >= 4) { s += h; s += '*'; s += std::to_string(j - i); s += ';'; }
		else s.append(j - i, h);
		i = j;
	}
	return s;
}

bool ramFromText(const char *t, std::vector<uint8_t> &ram)
{
	auto hexv = [](char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; };
	ram.clear();
	while (*t) {
		const int h = hexv(*t++);
		if (h < 0) return false;
		size_t count = 1;
		if (*t == '*') {
			char *end;
			count = size_t(strtoul(t + 1, &end, 10));
			if (*end != ';') return false;
			t = end + 1;
		}
		if (ram.size() + count > size_t(Machine::kRamNibbles)) return false;
		ram.insert(ram.end(), count, uint8_t(h));
	}
	return ram.size() == size_t(Machine::kRamNibbles);
}

void set_param(void *inst, const char *key, const char *val)
{
	auto *p = static_cast<Inst *>(inst);
	if (!strcmp(key, "state")) {
		std::vector<uint8_t> ram;
		if (p->romOk && ramFromText(val, ram)) {
			p->machine.setRam(ram.data(), ram.size());
			p->machine.powerOn();
		}
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
		const std::string s = ramText(p->machine.ram());
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
			return out(buf, len, p->valueText(t, p->v[t]));
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
	if (idx == P_K_STARTSTOP)
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
		const int16_t q = int16_t(std::lrintf(std::clamp(mono[size_t(i)], -1.0f, 1.0f) * 32767.0f));
		out_lr[2 * i] = q;
		out_lr[2 * i + 1] = q;
	}
}

const mpc_engine_t kEngine = {create, destroy, midi, set_param, get_param, render, nullptr, nullptr};

} // namespace

extern "C" const mpc_engine_t *mpc_engine(void) { return &kEngine; }
