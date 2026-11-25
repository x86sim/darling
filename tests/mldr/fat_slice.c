// Copyright (C) 2026 Lubos Dolezel
//
// Host-side test of mldr's universal-binary slice selection: 32-bit and
// 64-bit fat headers in both byte orders, architecture preferences, forced
// architecture, native/fallback selection, and malformed headers (too many
// architectures, a table or slice that does not fit the file, empty slices,
// slices inside the table, offsets that wrap).
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "fat_slice.h"

#define X86 7u
#define X86_64 0x01000007u
#define ARM64 0x0100000cu

static int failures;
static void check(int ok, const char* what)
{
	printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
	if (!ok)
		failures++;
}

struct arch { uint32_t cpu, sub; uint64_t off, size; };

static void put32(uint8_t* p, uint32_t v, int big)
{
	for (int i = 0; i < 4; i++)
		p[big ? 3 - i : i] = (uint8_t) (v >> (8 * i));
}
static void put64(uint8_t* p, uint64_t v, int big)
{
	put32(p + (big ? 0 : 4), (uint32_t) (v >> 32), big);
	put32(p + (big ? 4 : 0), (uint32_t) v, big);
}

// builds a fat header; returns its length
static size_t build(uint8_t* buf, int big, int wide, uint32_t count_field, const struct arch* a, unsigned n)
{
	memset(buf, 0, MLDR_FAT_MAX_HEADER + 64);
	if (big)
	{
		buf[0] = 0xca; buf[1] = 0xfe; buf[2] = 0xba; buf[3] = wide ? 0xbf : 0xbe;
	}
	else
	{
		buf[0] = wide ? 0xbf : 0xbe; buf[1] = 0xba; buf[2] = 0xfe; buf[3] = 0xca;
	}
	put32(buf + 4, count_field, big);
	unsigned w = wide ? 32 : 20;
	for (unsigned i = 0; i < n; i++)
	{
		uint8_t* r = buf + 8 + i * w;
		put32(r, a[i].cpu, big);
		put32(r + 4, a[i].sub, big);
		if (wide) { put64(r + 8, a[i].off, big); put64(r + 16, a[i].size, big); put32(r + 24, 12, big); }
		else { put32(r + 8, (uint32_t) a[i].off, big); put32(r + 12, (uint32_t) a[i].size, big); put32(r + 16, 12, big); }
	}
	return 8 + n * w;
}

static enum mldr_fat_result sel(const uint8_t* b, size_t len, uint64_t fsize, const uint32_t* prefs,
		uint32_t forced, struct mldr_fat_slice* out)
{
	return mldr_fat_select(b, len, fsize, prefs, forced, X86_64, X86, out);
}

int main(void)
{
	uint8_t buf[MLDR_FAT_MAX_HEADER + 64];
	struct mldr_fat_slice s;
	const uint32_t nopref[4] = {0};
	const struct arch two[] = { { X86, 3, 0x1000, 0x100 }, { X86_64, 3, 0x2000, 0x200 } };

	for (int big = 0; big <= 1; big++)
		for (int wide = 0; wide <= 1; wide++)
		{
			char name[96];
			size_t n = build(buf, big, wide, 2, two, 2);
			snprintf(name, sizeof name, "big=%d wide=%d: native x86_64 beats x86", big, wide);
			check(sel(buf, n, 0x3000, nopref, 0, &s) == MLDR_FAT_OK && s.cputype == X86_64 &&
					s.offset == 0x2000 && s.size == 0x200 && s.cpusubtype == 3, name);
			snprintf(name, sizeof name, "big=%d wide=%d: magic recognised", big, wide);
			check(mldr_fat_is_magic(buf), name);
		}

	// a 64-bit header can place a slice beyond 4 GiB
	{
		struct arch far[] = { { X86_64, 3, 0x100000000ull, 0x1000 } };
		size_t n = build(buf, 1, 1, 1, far, 1);
		check(sel(buf, n, 0x100001000ull, nopref, 0, &s) == MLDR_FAT_OK && s.offset == 0x100000000ull,
				"fat64 slice above 4 GiB");
		check(sel(buf, n, 0xfffff000ull, nopref, 0, &s) == MLDR_FAT_NO_MATCH,
				"fat64 slice beyond the end of the file is never selected");
	}

	// fallback: only x86 present on an x86_64 host; two x86 slices pick the first
	{
		struct arch x86only[] = { { X86, 3, 0x1000, 0x10 }, { X86, 4, 0x2000, 0x10 } };
		size_t n = build(buf, 1, 0, 2, x86only, 2);
		check(sel(buf, n, 0x3000, nopref, 0, &s) == MLDR_FAT_OK && s.offset == 0x1000, "x86 fallback, first slice");
		struct arch arm[] = { { ARM64, 0, 0x1000, 0x10 } };
		n = build(buf, 1, 0, 1, arm, 1);
		check(sel(buf, n, 0x2000, nopref, 0, &s) == MLDR_FAT_NO_MATCH, "no runnable architecture");
	}

	// preferences: earliest preference wins, first among equals; forced wins over everything
	{
		struct arch three[] = { { X86_64, 3, 0x1000, 0x10 }, { X86, 3, 0x2000, 0x10 }, { X86, 4, 0x3000, 0x10 } };
		size_t n = build(buf, 1, 0, 3, three, 3);
		const uint32_t prefs[4] = { 0, X86, X86_64, 0 };
		check(sel(buf, n, 0x4000, prefs, 0, &s) == MLDR_FAT_OK && s.cputype == X86 && s.offset == 0x2000, "preference order");
		check(sel(buf, n, 0x4000, prefs, X86_64, &s) == MLDR_FAT_OK && s.cputype == X86_64, "forced architecture");
		check(sel(buf, n, 0x4000, prefs, ARM64, &s) == MLDR_FAT_NO_MATCH, "forced architecture absent");
	}

	// malformed headers
	{
		struct arch ok[] = { { X86_64, 3, 0x1000, 0x10 } };
		size_t n = build(buf, 1, 0, 0xffffffffu, ok, 1);
		check(sel(buf, n, 0x2000, nopref, 0, &s) == MLDR_FAT_MALFORMED, "huge architecture count");
		n = build(buf, 1, 0, MLDR_FAT_MAX_ARCHES + 1, ok, 1);
		check(sel(buf, n, 1u << 20, nopref, 0, &s) == MLDR_FAT_MALFORMED, "count above the cap");
		build(buf, 1, 0, 5, ok, 1);
		check(sel(buf, 8 + 20, 8 + 20, nopref, 0, &s) == MLDR_FAT_MALFORMED, "table longer than the file");
		check(sel(buf, 6, 6, nopref, 0, &s) == MLDR_FAT_MALFORMED, "file shorter than the header");
		check(sel(buf, 3, 3, nopref, 0, &s) == MLDR_FAT_NOT_FAT, "fewer than four bytes");
		uint8_t notfat[16] = { 0xcf, 0xfa, 0xed, 0xfe };
		check(sel(notfat, 16, 16, nopref, 0, &s) == MLDR_FAT_NOT_FAT, "thin Mach-O is not fat");

		struct arch bad[] = { { X86_64, 3, 0x1000, 0 } };
		n = build(buf, 1, 0, 1, bad, 1);
		check(sel(buf, n, 0x2000, nopref, 0, &s) == MLDR_FAT_NO_MATCH, "empty slice");
		struct arch intable[] = { { X86_64, 3, 4, 0x10 } };
		n = build(buf, 1, 0, 1, intable, 1);
		check(sel(buf, n, 0x2000, nopref, 0, &s) == MLDR_FAT_NO_MATCH, "slice inside the header");
		struct arch past[] = { { X86_64, 3, 0x1000, 0x2000 } };
		n = build(buf, 1, 0, 1, past, 1);
		check(sel(buf, n, 0x2000, nopref, 0, &s) == MLDR_FAT_NO_MATCH, "slice running past the end");
		struct arch wrap[] = { { X86_64, 3, 0xfffffffffffff000ull, 0x2000 } };
		n = build(buf, 1, 1, 1, wrap, 1);
		check(sel(buf, n, 0x2000, nopref, 0, &s) == MLDR_FAT_NO_MATCH, "offset plus size wraps");

		// a bad record does not hide a good one
		struct arch mixed[] = { { X86_64, 3, 0x1000, 0x2000 }, { X86, 3, 0x1000, 0x100 } };
		n = build(buf, 1, 0, 2, mixed, 2);
		check(sel(buf, n, 0x2000, nopref, 0, &s) == MLDR_FAT_OK && s.cputype == X86, "invalid record skipped, valid one used");

		// output untouched on failure
		s.offset = 0x1234; s.size = 0x5678;
		n = build(buf, 1, 0, 0xffffffffu, ok, 1);
		sel(buf, n, 0x2000, nopref, 0, &s);
		check(s.offset == 0x1234 && s.size == 0x5678, "output untouched on error");
	}

	// which programs run under the arm64 translator (mldr_runs_as_arm64)
	{
		uint8_t thin_a64[8], thin_x64[8], fat[MLDR_FAT_MAX_HEADER + 64], fat_a64[MLDR_FAT_MAX_HEADER + 64];
		put32(thin_a64, 0xfeedfacfu, 0); put32(thin_a64 + 4, ARM64, 0);
		put32(thin_x64, 0xfeedfacfu, 0); put32(thin_x64 + 4, X86_64, 0);
		const struct arch universal[] = { { X86_64, 3, 0x1000, 0x100 }, { ARM64, 0, 0x2000, 0x100 } };
		const struct arch a64_armv7[] = { { 12, 9, 0x1000, 0x100 }, { ARM64, 0, 0x2000, 0x100 } };
		size_t fn = build(fat, 1, 0, 2, universal, 2);
		size_t an = build(fat_a64, 1, 0, 2, a64_armv7, 2);
		const uint32_t pref_a64[4] = { ARM64, 0, 0, 0 };

		check(mldr_runs_as_arm64(thin_a64, 8, 0x3000, nopref, 0, NULL, 0, 0), "thin arm64 runs as arm64");
		check(!mldr_runs_as_arm64(thin_x64, 8, 0x3000, nopref, 1, thin_a64, 8, 0x3000), "thin x86_64 runs natively");
		check(mldr_runs_as_arm64(fat_a64, an, 0x3000, nopref, 0, NULL, 0, 0), "arm64 + armv7 runs as arm64");
		// issue 26: a universal app in the arm64-only iOS 27 runtime aborted in dyld (dyld_sim incompatible)
		check(mldr_runs_as_arm64(fat, fn, 0x3000, nopref, 1, thin_a64, 8, 0x3000), "universal, arm64 runtime: arm64");
		check(mldr_runs_as_arm64(fat, fn, 0x3000, nopref, 1, fat_a64, an, 0x3000), "universal, fat arm64-only dyld_sim: arm64");
		check(mldr_runs_as_arm64(fat, fn, 0x3000, nopref, 1, NULL, 0, 0), "universal, unreadable dyld_sim: arm64");
		check(!mldr_runs_as_arm64(fat, fn, 0x3000, nopref, 1, thin_x64, 8, 0x3000), "universal, x86_64 runtime: x86_64");
		check(!mldr_runs_as_arm64(fat, fn, 0x3000, nopref, 1, fat, fn, 0x3000), "universal, universal runtime: x86_64");
		check(mldr_runs_as_arm64(fat, fn, 0x3000, pref_a64, 1, fat, fn, 0x3000), "universal, universal runtime, arm64 binpref: arm64");
		check(!mldr_runs_as_arm64(fat, fn, 0x3000, nopref, 0, NULL, 0, 0), "universal, not a simulator process: x86_64");
		check(!mldr_runs_as_arm64(thin_a64, 4, 0x3000, nopref, 0, NULL, 0, 0), "truncated header: native");
	}

	return failures ? 1 : 0;
}
