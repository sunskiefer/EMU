// EMU: finds a ROM by its SHA1 in a folder, whatever the file is called.
// The user supplies the ROMs; nothing here ships any. Looks in `dir` and one level of sub-folders.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace emu {

struct Sha1 {
	uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
	uint64_t len = 0;
	uint8_t buf[64];
	size_t n = 0;
	static uint32_t rol(uint32_t v, int s) { return (v << s) | (v >> (32 - s)); }
	void block(const uint8_t *p)
	{
		uint32_t w[80];
		for (int i = 0; i < 16; ++i)
			w[i] = uint32_t(p[i * 4]) << 24 | uint32_t(p[i * 4 + 1]) << 16 | uint32_t(p[i * 4 + 2]) << 8 | p[i * 4 + 3];
		for (int i = 16; i < 80; ++i)
			w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
		uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
		for (int i = 0; i < 80; ++i) {
			uint32_t f, k;
			if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
			else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
			else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
			else { f = b ^ c ^ d; k = 0xCA62C1D6; }
			const uint32_t t = rol(a, 5) + f + e + k + w[i];
			e = d; d = c; c = rol(b, 30); b = a; a = t;
		}
		h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
	}
	void update(const uint8_t *p, size_t m)
	{
		len += m;
		while (m) {
			const size_t t = std::min(m, 64 - n);
			memcpy(buf + n, p, t);
			n += t; p += t; m -= t;
			if (n == 64) { block(buf); n = 0; }
		}
	}
	std::string hex()
	{
		const uint64_t bits = len * 8;
		const uint8_t one = 0x80, zero = 0;
		update(&one, 1);
		while (n != 56) update(&zero, 1);
		uint8_t l[8];
		for (int i = 0; i < 8; ++i) l[i] = uint8_t(bits >> (56 - 8 * i));
		update(l, 8);
		char s[41];
		for (int i = 0; i < 5; ++i) snprintf(s + i * 8, 9, "%08x", h[i]);
		return s;
	}
};

inline bool readFile(const std::string &path, std::vector<uint8_t> &out)
{
	FILE *f = fopen(path.c_str(), "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	const long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (sz < 0 || sz > (64L << 20)) { fclose(f); return false; }
	out.resize(size_t(sz));
	const bool ok = fread(out.data(), 1, out.size(), f) == out.size();
	fclose(f);
	return ok;
}

inline std::string sha1Of(const std::vector<uint8_t> &d)
{
	Sha1 s;
	s.update(d.data(), d.size());
	return s.hex();
}

// Returns true and fills `out` when a file of exactly `size` bytes with this SHA1 is in dir (or a sub-folder of it).
inline bool findRom(const std::string &dir, const char *sha1hex, size_t size, std::vector<uint8_t> &out, int depth = 1)
{
	DIR *d = opendir(dir.c_str());
	if (!d) return false;
	std::vector<std::string> subs;
	bool found = false;
	while (dirent *e = readdir(d)) {
		if (e->d_name[0] == '.') continue;
		const std::string p = dir + "/" + e->d_name;
		struct stat st;
		if (stat(p.c_str(), &st) != 0) continue;
		if (S_ISDIR(st.st_mode)) { subs.push_back(p); continue; }
		if (size_t(st.st_size) != size) continue;
		std::vector<uint8_t> data;
		if (readFile(p, data) && sha1Of(data) == sha1hex) { out.swap(data); found = true; break; }
	}
	closedir(d);
	if (!found && depth > 0)
		for (const auto &s : subs)
			if (findRom(s, sha1hex, size, out, depth - 1)) return true;
	return found;
}

} // namespace emu
