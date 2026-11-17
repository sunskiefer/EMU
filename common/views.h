// EMU: the page/overview state and the Q-Link set that follows the page, shared by every plugin.
// Include after the plugin's generated spec_keys.h. Overview tiles open a page, OVERVIEW and < > in the header,
// ql1..ql16 drive the controls of the page on screen (names and values follow).
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace emu {

inline int optionCount(int idx)
{
	const char *o = kOpts[idx];
	if (!*o) return 0;
	int n = 1;
	for (; *o; ++o) n += (*o == '|');
	return n;
}

inline std::string optionName(int idx, int i)
{
	const char *o = kOpts[idx];
	for (; i > 0 && *o; ++o) if (*o == '|') --i;
	std::string s;
	for (; *o && *o != '|'; ++o) s += *o;
	return s;
}

// view / in_sec / nav / go keys. Returns true if idx was one of them (and v[] updated).
#ifdef EMU_ONEPAGE
inline bool viewSet(double *, int, double) { return false; }
inline int qlTarget(const double *, int q) { return kQl[1][q]; }
#else
inline bool viewSet(double *v, int idx, double x)
{
	if (idx == P_VIEW) { v[P_VIEW] = x < 0 ? 0 : x > N_PAGES ? N_PAGES : int(x + 0.5); v[P_IN_SEC] = v[P_VIEW] > 0; return true; }
	if (idx == P_IN_SEC) { return true; }
	if (idx == P_NAV_ZOOM || idx == P_NAV_PREV || idx == P_NAV_NEXT || (idx >= P_GO_1 && idx < P_GO_1 + N_PAGES)) {
		if (x <= 0.5) return true;
		int w = int(v[P_VIEW]);
		if (idx == P_NAV_ZOOM) w = 0;
		else if (idx == P_NAV_PREV) w = w <= 1 ? N_PAGES : w - 1;
		else if (idx == P_NAV_NEXT) w = w >= N_PAGES ? 1 : w + 1;
		else w = idx - P_GO_1 + 1;
		v[P_VIEW] = w;
		v[P_IN_SEC] = w > 0;
		return true;
	}
	return false;
}

inline int qlTarget(const double *v, int q) { return kQl[int(v[P_VIEW])][q]; }
#endif

// value text for any control: options by name, others as a number
inline std::string valueText(int idx, double x)
{
	if (optionCount(idx)) {
		int i = int(x + 0.5);
		if (i < 0) i = 0;
		if (i >= optionCount(idx)) i = optionCount(idx) - 1;
		return optionName(idx, i);
	}
	char b[32];
	snprintf(b, sizeof b, kMax[idx] - kMin[idx] > 20 ? "%.0f" : "%.1f", x);
	return b;
}

} // namespace emu
