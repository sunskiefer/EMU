// QVRBGT MPC engine: the Lemus QuadraVerb GT core (vendor/lemus/qvgt/core, AGPL-3.0, by lemus-dev) as an effect behind the
// mpc-vst-plugins mpc_engine() contract. The real firmware (v1.03, the user's own qvgtv103.bin in <plugin folder>/roms, found by
// SHA-1) runs on the emulated 80C31 and drives a model of the DASP 24 effects DSP; the analog preamp stages are circuit models.
// The unit's battery-backed RAM (programs) is kept in <plugin folder>/roms/qvgt-ram.bin, written after STORE and when the plugin closes.
// Without the ROM the effect passes the signal through. Not measured on a Force (see docs/NOTES.md). L'Cronx 2026.
// Parameter order = VST index: never reorder, only append (params.lock).
extern "C" {
#include "engine.h"
}
#include "spec_keys.h"
#include "views.h"
#include "romfind.h"
#include "unit.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr int kKeyHoldSamples = 5292; // a key stays down 120 ms
const int kKeyOf[16] = {qvgt::Unit::kKeyProgram, qvgt::Unit::kKeyEq, qvgt::Unit::kKeyPreAmp, qvgt::Unit::kKeyPitch, qvgt::Unit::kKeyDelay,
                        qvgt::Unit::kKeyReverb, qvgt::Unit::kKeyMix, qvgt::Unit::kKeyConfig, qvgt::Unit::kKeyValueDown, qvgt::Unit::kKeyValueUp,
                        qvgt::Unit::kKeyPageDown, qvgt::Unit::kKeyPageUp, qvgt::Unit::kKeyStore, qvgt::Unit::kKeyNameMod, qvgt::Unit::kKeyMidi,
                        qvgt::Unit::kKeyBypass};

struct Inst {
	std::unique_ptr<qvgt::Unit> unit;
	std::string dataDir, ramPath;
	bool romOk = false, ramDirty = false;
	double rate = 44100;
	double v[P_COUNT] = {};
	int hold[16] = {};
	std::string info;
	char status[96] = "", lcd[2][40] = {"", ""};
	std::chrono::steady_clock::time_point dirtyAt;

	Inst()
	{
		for (int i = 0; i < P_COUNT; ++i)
			v[i] = kDef[i];
	}

	void init()
	{
		std::vector<uint8_t> rom;
		if (dataDir.empty() || !emu::findRom(dataDir, "0ac063e74b6c6ecf9edda10824df9a79c621ccd9", 65536, rom) || !qvgt::Board::prepareRom(rom))
			return;
		unit = std::make_unique<qvgt::Unit>(std::move(rom));
		ramPath = dataDir + "/qvgt-ram.bin";
		bool haveRam = false;
		if (FILE *f = fopen(ramPath.c_str(), "rb")) {
			auto &ram = unit->board().ram();
			haveRam = fread(ram.data(), 1, ram.size(), f) == ram.size();
			fclose(f);
		}
		unit->prepare(rate);
		unit->powerOn();
		if (!haveRam) { // the first time: the unit's 90 factory programs the way the unit does it (STORE, PAGE UP x3, STORE)
			unit->recallFactoryPrograms();
			saveRam();
		}
		romOk = true;
	}

	void saveRam()
	{
		if (!unit || ramPath.empty())
			return;
		const std::string tmp = ramPath + ".new";
		if (FILE *f = fopen(tmp.c_str(), "wb")) {
			const auto &ram = unit->board().ram();
			const bool ok = fwrite(ram.data(), 1, ram.size(), f) == ram.size();
			fclose(f);
			if (ok) rename(tmp.c_str(), ramPath.c_str());
		}
		ramDirty = false;
	}

	void press(int slot)
	{
		if (!unit)
			return;
		unit->wake();
		unit->board().setKey(kKeyOf[slot], true);
		hold[slot] = kKeyHoldSamples;
		if (kKeyOf[slot] == qvgt::Unit::kKeyStore) {
			ramDirty = true;
			dirtyAt = std::chrono::steady_clock::now();
		}
	}

	void doMidi(const uint8_t *m, int len)
	{
		if (!unit)
			return;
		unit->wake();
		for (int i = 0; i < len; ++i)
			unit->board().midiIn(m[i]);
	}

	void process(const int16_t *in, int16_t *out, int frames)
	{
		if (!unit) {
			memcpy(out, in, size_t(frames) * 2 * sizeof(int16_t));
			return;
		}
		unit->inputPot = v[P_INPUT] / 10;
		unit->outputPot = v[P_OUTPUT] / 10;
		static thread_local std::vector<float> l, r, ol, orr;
		l.resize(size_t(frames)); r.resize(size_t(frames)); ol.resize(size_t(frames)); orr.resize(size_t(frames));
		for (int i = 0; i < frames; ++i) {
			l[size_t(i)] = in[2 * i] / 32768.0f;
			r[size_t(i)] = in[2 * i + 1] / 32768.0f;
		}
		unit->process(l.data(), r.data(), ol.data(), orr.data(), frames);
		for (int i = 0; i < frames; ++i) {
			out[2 * i] = int16_t(std::lrintf(std::clamp(ol[size_t(i)], -1.0f, 1.0f) * 32767.0f));
			out[2 * i + 1] = int16_t(std::lrintf(std::clamp(orr[size_t(i)], -1.0f, 1.0f) * 32767.0f));
		}
		for (int k = 0; k < 16; ++k)
			if (hold[k] > 0 && (hold[k] -= frames) <= 0) {
				hold[k] = 0;
				unit->board().setKey(kKeyOf[k], false);
			}
	}

	void poll()
	{
		if (!unit) {
			snprintf(status, sizeof status, "NO FIRMWARE ROM");
			snprintf(lcd[0], sizeof lcd[0], "NO qvgtv103.bin");
			snprintf(lcd[1], sizeof lcd[1], "IN ROMS FOLDER");
			return;
		}
		for (int n = 0; n < 2; ++n)
			snprintf(lcd[n], sizeof lcd[n], "%s", unit->board().lcd().text(n).c_str());
		const int h = unit->headroom();
		snprintf(status, sizeof status, "LEVEL %d/4%s%s", h, unit->clip() ? " CLIP" : "", unit->asleep() ? " (idle)" : "");
		if (ramDirty && std::chrono::steady_clock::now() - dirtyAt > std::chrono::seconds(2))
			saveRam();
	}
};

void *create(const char *dataDir)
{
	auto *p = new Inst();
	if (dataDir)
		p->dataDir = dataDir;
	p->init();
	p->poll();
	return p;
}

void destroy(void *i)
{
	auto *p = static_cast<Inst *>(i);
	if (p->unit)
		p->saveRam();
	delete p;
}

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
	if (idx == P_LCD1 || idx == P_LCD2 || idx == P_STATUS || idx == P_INFO || idx == P_BRAND || (idx >= P_QL1 && idx < P_QL1 + 16))
		return;
	if (idx >= P_K_PROGRAM && idx < P_K_PROGRAM + 16) {
		if (x > 0.5) {
			p->info = kNames[idx];
			p->press(idx - P_K_PROGRAM);
		}
		return;
	}
	p->info = std::string(kNames[idx]) + "  " + emu::valueText(idx, x);
	p->v[idx] = std::clamp(x, kMin[idx], kMax[idx]);
}

void set_param(void *inst, const char *key, const char *val)
{
	auto *p = static_cast<Inst *>(inst);
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
	if (idx == P_STATUS || idx == P_LCD1 || idx == P_LCD2) {
		p->poll();
		return out(buf, len, idx == P_STATUS ? p->status : p->lcd[idx == P_LCD1 ? 0 : 1]);
	}
	if (idx == P_INFO)
		return out(buf, len, p->info.empty() ? "TOUCH A CONTROL" : p->info);
	if (idx == P_BRAND)
		return out(buf, len, "L'Cronx");
	return snprintf(buf, size_t(len), "%g", p->v[idx]);
}

void process(void *inst, const int16_t *in_lr, int16_t *out_lr, int frames) { static_cast<Inst *>(inst)->process(in_lr, out_lr, frames); }

const mpc_engine_t kEngine = {create, destroy, midi, set_param, get_param, nullptr, process, nullptr};

} // namespace

extern "C" const mpc_engine_t *mpc_engine(void) { return &kEngine; }
