// 909MPC engine: the Lemus TR-909 core (vendor/lemus/tr909/core, AGPL-3.0, by lemus-dev) behind the mpc-vst-plugins mpc_engine()
// contract. The sequencer is the original v4.0 firmware on an emulated uPD7811, the voices are the voicing board solved from its
// netlists. Both need the user's own ROMs in <plugin folder>/roms (found by SHA-1, any file name); without the program ROM the
// voices play from MIDI (voice mode), without a sample ROM that cymbal/hat is silent.
// Not measured on a Force: the board is a circuit solver and may not run in real time on MPC OS (see docs/NOTES.md).
// L'Cronx 2026. Parameter order = VST index: never reorder, only append (params.lock).
extern "C" {
#include "engine.h"
}
#include "spec_keys.h"
#include "views.h"
#include "romfind.h"
#include "tr909/core/factory.h"
#include "tr909/core/machine.h"
#include "voiceboard.h"
#include "parallel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

using tr909::Machine;
using tr909::Voice;

constexpr double kWarmSeconds = 1.0;
constexpr int kKeyHoldSamples = 5292; // a panel key stays down 120 ms: the firmware scans and debounces
constexpr int kTrigReg[tr909::kVoices] = {9, 9, 9, 9, 9, 10, 10, 10, 10, 10};
constexpr int kTrigBit[tr909::kVoices] = {1, 2, 3, 4, 5, 1, 6, 2, 4, 5};

const std::pair<int, Machine::Key> kKeyMap[] = {
	{P_K_START, Machine::Key::Start}, {P_K_STOPCONT, Machine::Key::StopCont},
	{P_K_TRK1, Machine::Key::Track1}, {P_K_TRK2, Machine::Key::Track2}, {P_K_TRK3, Machine::Key::Track3}, {P_K_TRK4, Machine::Key::Track4},
	{P_K_PAT1, Machine::Key::Pattern1}, {P_K_PAT2, Machine::Key::Pattern2}, {P_K_PAT3, Machine::Key::Pattern3}, {P_K_EXT, Machine::Key::ExtInst},
	{P_K_TEMPO, Machine::Key::Tempo}, {P_K_BACK, Machine::Key::Back}, {P_K_FWD, Machine::Key::Fwd}, {P_K_MEAS, Machine::Key::AvailableMeas},
	{P_K_CYCLE, Machine::Key::Cycle}, {P_K_TAPE, Machine::Key::TapeSync}, {P_K_ENTER, Machine::Key::EnterAccent},
	{P_K_LAST, Machine::Key::LastStep}, {P_K_SCALE, Machine::Key::Scale}, {P_K_SHUFFLE, Machine::Key::ShuffleFlam},
	{P_K_CLEAR, Machine::Key::Clear}, {P_K_INST, Machine::Key::InstSelect}, {P_K_SHIFT, Machine::Key::Shift},
};

// the panel knobs, in tr909::Controls (accent and tempo are the CPU's pots)
struct KnobMap { int p; double tr909::Controls::*field; };
const KnobMap kKnobs[] = {
	{P_BD_TUNE, &tr909::Controls::bdTune}, {P_BD_LEVEL, &tr909::Controls::bdLevel}, {P_BD_ATTACK, &tr909::Controls::bdAttack}, {P_BD_DECAY, &tr909::Controls::bdDecay},
	{P_SD_TUNE, &tr909::Controls::sdTune}, {P_SD_LEVEL, &tr909::Controls::sdLevel}, {P_SD_TONE, &tr909::Controls::sdTone}, {P_SD_SNAPPY, &tr909::Controls::sdSnappy},
	{P_LT_TUNE, &tr909::Controls::ltTune}, {P_LT_LEVEL, &tr909::Controls::ltLevel}, {P_LT_DECAY, &tr909::Controls::ltDecay},
	{P_MT_TUNE, &tr909::Controls::mtTune}, {P_MT_LEVEL, &tr909::Controls::mtLevel}, {P_MT_DECAY, &tr909::Controls::mtDecay},
	{P_HT_TUNE, &tr909::Controls::htTune}, {P_HT_LEVEL, &tr909::Controls::htLevel}, {P_HT_DECAY, &tr909::Controls::htDecay},
	{P_RS_LEVEL, &tr909::Controls::rsLevel}, {P_HC_LEVEL, &tr909::Controls::hcLevel},
	{P_HH_LEVEL, &tr909::Controls::hhLevel}, {P_CH_DECAY, &tr909::Controls::chDecay}, {P_OH_DECAY, &tr909::Controls::ohDecay},
	{P_CR_LEVEL, &tr909::Controls::crLevel}, {P_CR_TUNE, &tr909::Controls::crTune}, {P_RD_LEVEL, &tr909::Controls::rdLevel}, {P_RD_TUNE, &tr909::Controls::rdTune},
	{P_VOLUME, &tr909::Controls::volume},
};

// v4.0 firmware: tempo value from the pot (docs/firmware.md, "Sync"), for the readout
double tempoBpmOf(int value)
{
	const int b = 32 + 225 * std::clamp(value, 0, 255) / 256;
	return 60e6 / (48.0 * double(1099138 / b));
}
double tempoBpm(double v)
{
	const int code = int(std::lround(std::clamp(v, 0.0, 1.0) * 255.0));
	return tempoBpmOf(code <= 126 ? code + 1 : code >= 130 ? code - 1 : 128);
}

int voiceOfNote(int note)
{
	switch (note) {
	case 35: case 36: return int(Voice::BD);
	case 38: case 40: return int(Voice::SD);
	case 41: case 43: return int(Voice::LT);
	case 45: case 47: return int(Voice::MT);
	case 48: case 50: return int(Voice::HT);
	case 37: return int(Voice::RS);
	case 39: return int(Voice::HC);
	case 42: case 44: case 46: return int(Voice::HH);
	case 49: return int(Voice::CR);
	case 51: return int(Voice::RD);
	default: return -1;
	}
}

struct Inst {
	Machine machine;
	tr909::VoiceBoard board;
	std::unique_ptr<tr909::Parallel> parallel;
	std::vector<uint8_t> hihat, crash, ride;
	std::unique_ptr<std::array<uint8_t, Machine::kRamSize>> factory;
	std::string dataDir;
	bool romOk = false;
	double rate = 44100, stateAcc = 0;
	double v[P_COUNT] = {};
	int hold[int(Machine::Key::Count)] = {};
	bool shiftLatched = false, running = false, asleep = false;
	std::array<int, tr909::kVoices> voiceTrig{};
	tr909::Controls lastControls;
	bool haveControls = false;
	std::string info;
	char status[96] = "";
	int trigSamples = 88;

	Inst()
	{
		for (int i = 0; i < P_COUNT; ++i)
			v[i] = kDef[i];
	}

	static bool loadRomBlob(const std::string &dir, const char *sha1, size_t size, std::vector<uint8_t> &out)
	{
		return !dir.empty() && emu::findRom(dir, sha1, size, out);
	}

	void init()
	{
		std::vector<uint8_t> prog, v1;
		romOk = loadRomBlob(dataDir, "b94374e2cd6b5a6c405da4fa7793c98c0a6015c4", 8192, prog) && machine.setRom(prog.data(), prog.size());
		loadRomBlob(dataDir, "22a34d603e78673bb096f5e56a8971afe14d8fee", 32768, hihat);
		loadRomBlob(dataDir, "bc730107b0c391f4879c9777a3e85f3e57f3aad6", 32768, crash);
		loadRomBlob(dataDir, "daa54c58c7e3ae3398f125568537ec82d5bd1dfd", 32768, ride);
		tr909::SampleRoms roms{hihat.size() == 32768 ? hihat.data() : nullptr, crash.size() == 32768 ? crash.data() : nullptr,
		                       ride.size() == 32768 ? ride.data() : nullptr};
		board.setSampleRoms(roms);
		board.setCalibration(tr909::unitCalibration());
		if (loadRomBlob(dataDir, "bcd754900574d1861034af8056574ffa77db3067", 8192, v1)) {
			auto ram = std::make_unique<std::array<uint8_t, Machine::kRamSize>>();
			if (tr909::factoryRam(v1.data(), v1.size(), *ram))
				factory = std::move(ram);
		}
		// worker threads for the voices' jobs, as Lemus's plugin: two besides the audio thread when the machine has the cores
		const int cores = int(std::thread::hardware_concurrency());
		const int workers = std::clamp(cores - 2, 0, 2);
		parallel = std::make_unique<tr909::Parallel>(workers, [](int) {}, [](int) {});
		board.setParallel(parallel.get());
		if (factory)
			machine.setRam(factory->data(), factory->size());
		board.setControls(controls());
		board.prepare(rate);
		trigSamples = std::max(1, int(std::lround(0.002 * rate)));
		warmStart();
	}

	tr909::Controls controls() const
	{
		tr909::Controls c;
		for (const auto &k : kKnobs)
			c.*(k.field) = v[k.p] / 10;
		return c;
	}

	void warmStart()
	{
		machine.powerOn();
		running = false;
		if (!romOk)
			return;
		machine.setTempoPot(v[P_TEMPO] / 10);
		machine.setTotalAccentPot(v[P_ACCENT] / 10);
		machine.run(int(kWarmSeconds * Machine::kStateHz));
		machine.takeMidiOutTimed();
	}

	void voiceModeNote(int note, int velocity)
	{
		const int vc = voiceOfNote(note);
		if (vc < 0 || velocity <= 0)
			return;
		static constexpr int kDac[tr909::kVoices] = {1, 2, 3, 4, 5, 6, 6, 7, 8, 8};
		static constexpr int kAnalog[17] = {2, 3, 4, 5, 6, 8, 10, 12, 14, 17, 21, 27, 33, 41, 49, 57, 63};
		static constexpr int kSampled[17] = {1, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 63};
		const bool sampled = Voice(vc) == Voice::HH || Voice(vc) == Voice::CR || Voice(vc) == Voice::RD;
		const int *law = sampled ? kSampled : kAnalog;
		const int k = std::min(15, velocity / 8);
		const double x0 = 8.0 * k, x1 = k == 15 ? 127.0 : 8.0 * (k + 1);
		const int code = std::clamp(int(std::lround(law[k] + (law[k + 1] - law[k]) * (velocity - x0) / (x1 - x0))), 0, 63);
		board.write(kDac[vc], uint8_t(code << 1));
		if (Voice(vc) == Voice::HH)
			board.write(10, uint8_t((board.latch(10) & ~0x08) | (note == 46 ? 0 : 0x08)));
		board.write(kTrigReg[vc], uint8_t(board.latch(kTrigReg[vc]) | (1u << kTrigBit[vc])));
		voiceTrig[size_t(vc)] = trigSamples;
		asleep = false;
	}

	void doMidi(const uint8_t *m, int len)
	{
		if (len < 1)
			return;
		const int st = m[0] & 0xf0, d1 = len > 1 ? m[1] : 0, d2 = len > 2 ? m[2] : 0;
		if (st == 0xB0 && len > 2) {
			// Roland TR-8's control changes for the same controls; MASTER VOLUME is CC 7 (the 909 itself takes none)
			static const std::pair<int, int> kCc[] = {
				{20, P_BD_TUNE}, {21, P_BD_ATTACK}, {23, P_BD_DECAY}, {24, P_BD_LEVEL}, {25, P_SD_TUNE}, {26, P_SD_SNAPPY}, {28, P_SD_TONE},
				{29, P_SD_LEVEL}, {46, P_LT_TUNE}, {47, P_LT_DECAY}, {48, P_LT_LEVEL}, {49, P_MT_TUNE}, {50, P_MT_DECAY}, {51, P_MT_LEVEL},
				{52, P_HT_TUNE}, {53, P_HT_DECAY}, {54, P_HT_LEVEL}, {57, P_RS_LEVEL}, {60, P_HC_LEVEL}, {62, P_CH_DECAY}, {63, P_HH_LEVEL},
				{81, P_OH_DECAY}, {82, P_HH_LEVEL}, {83, P_CR_TUNE}, {85, P_CR_LEVEL}, {86, P_RD_TUNE}, {88, P_RD_LEVEL}, {71, P_ACCENT}, {7, P_VOLUME}};
			for (const auto &c : kCc)
				if (c.first == d1)
					v[c.second] = d2 / 127.0 * 10;
			return;
		}
		if (m[0] == 0xfe)
			return;
		if (!romOk) {
			if (st == 0x90 && d2 > 0)
				voiceModeNote(d1, d2);
			return;
		}
		machine.midiIn(m, size_t(len));
	}

	void pressKey(Machine::Key k)
	{
		if (!romOk)
			return;
		if (k == Machine::Key::Shift) { // SHIFT latches: a touch screen can't hold it while another key is pressed
			shiftLatched = !shiftLatched;
			machine.setKey(k, shiftLatched);
			return;
		}
		machine.setKey(k, true);
		hold[int(k)] = kKeyHoldSamples;
	}

	void latch(int reg, uint8_t value)
	{
		if (board.latch(reg) == value)
			return;
		board.write(reg, value);
		asleep = false;
	}

	void renderBoard(float *l, float *r, int n)
	{
		if (n <= 0)
			return;
		if (asleep) {
			std::fill(l, l + n, 0.0f);
			std::fill(r, r + n, 0.0f);
			return;
		}
		board.render(l, r, n);
	}

	void render(int16_t *out, int frames)
	{
		static thread_local std::vector<float> L, R;
		L.assign(size_t(frames), 0.0f);
		R.assign(size_t(frames), 0.0f);
		const auto c = controls();
		if (!haveControls || std::memcmp(&c, &lastControls, sizeof c) != 0) {
			board.setControls(c);
			lastControls = c;
			haveControls = true;
			asleep = false;
		}
		machine.setTempoPot(v[P_TEMPO] / 10);
		machine.setTotalAccentPot(v[P_ACCENT] / 10);
		int done = 0;
		for (int pos = 0; pos < frames;) {
			int next = frames;
			for (const int t : voiceTrig)
				if (t > 0)
					next = std::min(next, pos + t);
			for (const int h : hold)
				if (h > 0)
					next = std::min(next, pos + h);
			next = std::max(next, pos + 1);
			const int len = next - pos;
			if (romOk) {
				stateAcc += len * Machine::kStateHz / rate;
				const int states = int(stateAcc);
				stateAcc -= states;
				machine.run(states);
				for (const auto &w : machine.writes()) {
					const int at = pos + std::min(len - 1, int(double(w.state) * rate / Machine::kStateHz));
					if (board.latch(w.reg) == w.value)
						continue;
					if (at > done) {
						renderBoard(L.data() + done, R.data() + done, at - done);
						done = at;
					}
					latch(w.reg, w.value);
				}
				for (const auto &b : machine.takeMidiOutTimed()) {
					if (b.data == 0xfa || b.data == 0xfb) running = true;
					else if (b.data == 0xfc) running = false;
				}
			}
			for (int k = 0; k < int(Machine::Key::Count); ++k)
				if (hold[k] > 0 && (hold[k] -= len) <= 0) {
					hold[k] = 0;
					machine.setKey(Machine::Key(k), false);
				}
			for (int vc = 0; vc < tr909::kVoices; ++vc) {
				auto &t = voiceTrig[size_t(vc)];
				if (t <= 0)
					continue;
				t -= len;
				if (t <= 0) {
					t = 0;
					if (next > done) {
						renderBoard(L.data() + done, R.data() + done, next - done);
						done = next;
					}
					latch(kTrigReg[vc], uint8_t(board.latch(kTrigReg[vc]) & ~(1u << kTrigBit[vc])));
				}
			}
			pos = next;
		}
		renderBoard(L.data() + done, R.data() + done, frames - done);
		if (!asleep && board.asleep())
			asleep = true;
		for (int i = 0; i < frames; ++i) {
			out[2 * i] = int16_t(std::lrintf(std::clamp(L[size_t(i)], -1.0f, 1.0f) * 32767.0f));
			out[2 * i + 1] = int16_t(std::lrintf(std::clamp(R[size_t(i)], -1.0f, 1.0f) * 32767.0f));
		}
	}

	void updateStatus()
	{
		std::string s;
		if (!romOk) {
			s = "VOICE MODE (no program ROM)";
		} else {
			char d[4];
			for (int i = 0; i < 3; ++i) {
				const int x = machine.digit(i);
				d[i] = x <= 9 ? char('0' + x) : ' ';
			}
			d[3] = 0;
			s = std::string(running ? "RUN " : "STOP ") + d;
		}
		if (hihat.size() != 32768) s += " NO HH ROM";
		if (crash.size() != 32768) s += " NO CR ROM";
		if (ride.size() != 32768) s += " NO RD ROM";
		snprintf(status, sizeof status, "%s", s.c_str());
	}

	std::string valueText(int idx, double x) const
	{
		if (idx == P_TEMPO) {
			char b[32];
			snprintf(b, sizeof b, "%.1f BPM", tempoBpm(x / 10));
			return b;
		}
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

const Machine::Key *machineKey(int idx)
{
	for (const auto &km : kKeyMap)
		if (km.first == idx)
			return &km.second;
	return nullptr;
}

void setIdx(Inst *p, int idx, double x)
{
	if (emu::viewSet(p->v, idx, x))
		return;
	if (idx == P_STATUS || idx == P_INFO || idx == P_BRAND)
		return;
	if (idx >= P_QL1 && idx < P_QL1 + 16)
		return;
	if (const auto *k = machineKey(idx)) {
		if (x > 0.5) {
			p->info = kNames[idx];
			p->pressKey(*k);
		}
		return;
	}
	p->info = std::string(kNames[idx]) + "  " + p->valueText(idx, x);
	p->v[idx] = std::clamp(x, kMin[idx], kMax[idx]);
}

// the battery RAM as text, runs of the same byte as "hh*count;" (it is mostly one value, and the wrapper's state buffer is 8 KB)
std::string ramText(const std::array<uint8_t, Machine::kRamSize> &ram)
{
	std::string s;
	for (size_t i = 0; i < ram.size();) {
		size_t j = i;
		while (j < ram.size() && ram[j] == ram[i]) ++j;
		char b[24];
		if (j - i >= 4) snprintf(b, sizeof b, "%02x*%zu;", ram[i], j - i);
		else { b[0] = 0; for (size_t k = i; k < j; ++k) snprintf(b + strlen(b), 4, "%02x", ram[k]); }
		s += b;
		i = j;
	}
	return s;
}

bool ramFromText(const char *t, std::array<uint8_t, Machine::kRamSize> &ram)
{
	size_t n = 0;
	auto hexv = [](char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; };
	while (t[0] && t[1]) {
		const int h = hexv(t[0]), l = hexv(t[1]);
		if (h < 0 || l < 0) return false;
		const uint8_t byte = uint8_t(h * 16 + l);
		t += 2;
		size_t count = 1;
		if (*t == '*') {
			count = size_t(strtoul(t + 1, const_cast<char **>(&t), 10));
			if (*t != ';') return false;
			++t;
		}
		if (n + count > ram.size()) return false;
		memset(ram.data() + n, byte, count);
		n += count;
	}
	return n == ram.size();
}

void set_param(void *inst, const char *key, const char *val)
{
	auto *p = static_cast<Inst *>(inst);
	if (!strcmp(key, "state")) {
		std::array<uint8_t, Machine::kRamSize> ram;
		if (p->romOk && ramFromText(val, ram)) {
			p->machine.setRam(ram.data(), ram.size());
			p->warmStart();
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
	if (idx == P_K_SHIFT)
		return snprintf(buf, size_t(len), "%d", p->shiftLatched ? 1 : 0);
	return snprintf(buf, size_t(len), "%g", p->v[idx]);
}

void render(void *inst, int16_t *out_lr, int frames) { static_cast<Inst *>(inst)->render(out_lr, frames); }

const mpc_engine_t kEngine = {create, destroy, midi, set_param, get_param, render, nullptr, nullptr};

} // namespace

extern "C" const mpc_engine_t *mpc_engine(void) { return &kEngine; }
