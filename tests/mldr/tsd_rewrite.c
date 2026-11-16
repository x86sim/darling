/*
 * This file is part of Darling.
 *
 * Copyright (C) 2026 Lubos Dolezel
 *
 * Darling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Darling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Darling.  If not, see <http://www.gnu.org/licenses/>.
 */

// Host test of the ARM64 TSD read rewriting helpers (tsd_rewrite.h). It runs on
// any host: it only manipulates instruction words. The encodings are checked
// against an independent decoder written from the Arm Architecture Reference
// Manual for A-profile (DDI 0487): MRS (register), AND (immediate) with its
// DecodeBitMasks bitmask-immediate rule, LDR (immediate, unsigned offset; 64-
// and 32-bit), LDUR (32-bit) and MOVZ. A small model machine runs rewritten
// CPU number reads.
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "tsd_rewrite.h"

static int failures;

static void check(int ok, const char* what)
{
	printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
	if (!ok)
		failures++;
}

// --- reference decoders -----------------------------------------------------

struct mrs { int ok; unsigned op0, op1, crn, crm, op2, rt; };

// MRS Xt, <sysreg>: 1101 0101 0011 (o0=1) op1 CRn CRm op2 Rt; op0 = 2 + o0.
static struct mrs decode_mrs(uint32_t insn)
{
	struct mrs r = {0};
	if ((insn >> 20) != 0xD53)
		return r;
	r.ok = 1;
	r.op0 = 2 + ((insn >> 19) & 1);
	r.op1 = (insn >> 16) & 7;
	r.crn = (insn >> 12) & 15;
	r.crm = (insn >> 8) & 15;
	r.op2 = (insn >> 5) & 7;
	r.rt = insn & 31;
	return r;
}

static uint64_t ror64(uint64_t v, unsigned n, unsigned size)
{
	uint64_t mask = size == 64 ? ~0ull : (1ull << size) - 1;
	v &= mask;
	n %= size;
	return n ? ((v >> n) | (v << (size - n))) & mask : v;
}

// DecodeBitMasks for a 64-bit logical immediate (immediate = TRUE).
static int decode_bitmask(unsigned n, unsigned imms, unsigned immr, uint64_t* out)
{
	unsigned notimms = (~imms) & 63;
	unsigned combined = (n << 6) | notimms; // 7 bits
	int len = -1;
	for (int i = 6; i >= 0; i--)
		if (combined & (1u << i)) { len = i; break; }
	if (len < 1)
		return 0;
	unsigned esize = 1u << len;
	unsigned levels = esize - 1;
	unsigned s = imms & levels;
	unsigned r = immr & levels;
	if (s == levels)
		return 0;
	uint64_t welem = (1ull << (s + 1)) - 1;
	uint64_t elem = ror64(welem, r, esize);
	uint64_t result = 0;
	for (unsigned i = 0; i < 64; i += esize)
		result |= elem << i;
	*out = result;
	return 1;
}

struct and_imm { int ok; unsigned rd, rn; uint64_t imm; };

// AND Xd, Xn, #imm: 1 00 100100 N immr imms Rn Rd
static struct and_imm decode_and_x(uint32_t insn)
{
	struct and_imm r = {0};
	if ((insn >> 23) != ((1u << 8) | (0u << 7) | 0x24)) // sf=1 opc=00 100100
		return r;
	unsigned n = (insn >> 22) & 1;
	unsigned immr = (insn >> 16) & 63;
	unsigned imms = (insn >> 10) & 63;
	if (!decode_bitmask(n, imms, immr, &r.imm))
		return r;
	r.ok = 1;
	r.rn = (insn >> 5) & 31;
	r.rd = insn & 31;
	return r;
}

struct ldr_imm { int ok; unsigned rt, rn; uint64_t offset; };

// LDR Xt, [Xn, #pimm]: 11 111 0 01 01 imm12 Rn Rt, offset = imm12 * 8
static struct ldr_imm decode_ldr_x(uint32_t insn)
{
	struct ldr_imm r = {0};
	if ((insn >> 22) != 0x3E5)
		return r;
	r.ok = 1;
	r.offset = (uint64_t) ((insn >> 10) & 0xFFF) * 8;
	r.rn = (insn >> 5) & 31;
	r.rt = insn & 31;
	return r;
}

// AND Wd, Wn, #imm: 0 00 100100 N immr imms Rn Rd, N must be 0
static struct and_imm decode_and_w(uint32_t insn)
{
	struct and_imm r = {0};
	if ((insn >> 23) != 0x24 || ((insn >> 22) & 1) != 0) // sf=0 opc=00 100100 N=0
		return r;
	unsigned immr = (insn >> 16) & 63;
	unsigned imms = (insn >> 10) & 63;
	if ((imms & 0x20) || (immr & 0x20) || !decode_bitmask(0, imms, immr, &r.imm))
		return r;
	r.imm &= 0xFFFFFFFFull;
	r.ok = 1;
	r.rn = (insn >> 5) & 31;
	r.rd = insn & 31;
	return r;
}

struct ldr_w { int ok; unsigned rt, rn; int64_t offset; };

// LDR Wt, [Xn, #pimm]: 10 111 0 01 01 imm12 Rn Rt, offset = imm12 * 4
// LDUR Wt, [Xn, #simm]: 10 111 0 00 01 0 imm9 00 Rn Rt
static struct ldr_w decode_ldr_w(uint32_t insn)
{
	struct ldr_w r = {0};
	if ((insn >> 22) == 0x2E5)
		r.offset = (int64_t) ((insn >> 10) & 0xFFF) * 4;
	else if ((insn >> 21) == 0x5C2 && ((insn >> 10) & 3) == 0)
	{
		int32_t imm9 = (insn >> 12) & 0x1FF;
		r.offset = imm9 >= 256 ? imm9 - 512 : imm9;
	}
	else
		return r;
	r.ok = 1;
	r.rn = (insn >> 5) & 31;
	r.rt = insn & 31;
	return r;
}

struct movz { int ok; int sf; unsigned rd, hw, imm16; };

// MOVZ: sf 10 100101 hw imm16 Rd
static struct movz decode_movz(uint32_t insn)
{
	struct movz r = {0};
	if (((insn >> 23) & 0xFF) != 0xA5)
		return r;
	r.ok = 1;
	r.sf = insn >> 31;
	r.hw = (insn >> 21) & 3;
	r.imm16 = (insn >> 5) & 0xFFFF;
	r.rd = insn & 31;
	return r;
}

// Runs a two-word CPU number read on a model machine: the thread pointer is
// `tp`, and `memory` (based at `mem_base`) holds 32-bit words. Returns the
// final register file through `x`; 0 if an instruction is not one a rewritten
// pair may contain or a load leaves the model memory.
static int run_cpu_pair(const uint32_t pair[2], uint64_t tp, const uint32_t* memory, uint64_t mem_base, size_t mem_words, uint64_t x[32])
{
	for (int i = 0; i < 2; i++)
	{
		uint32_t insn = pair[i];
		struct mrs m = decode_mrs(insn);
		struct and_imm ax = decode_and_x(insn), aw = decode_and_w(insn);
		struct ldr_w l = decode_ldr_w(insn);
		struct movz z = decode_movz(insn);
		if (m.ok && m.op0 == 3 && m.op1 == 3 && m.crn == 13 && m.crm == 0 && m.op2 == 2)
			x[m.rt] = tp;
		else if (ax.ok)
			x[ax.rd] = x[ax.rn] & ax.imm;
		else if (aw.ok)
			x[aw.rd] = (uint32_t) x[aw.rn] & aw.imm;
		else if (l.ok)
		{
			uint64_t addr = x[l.rn] + (uint64_t) l.offset;
			if (addr < mem_base || (addr - mem_base) % 4 || (addr - mem_base) / 4 >= mem_words)
				return 0;
			x[l.rt] = memory[(addr - mem_base) / 4]; // 32-bit loads zero-extend
		}
		else if (z.ok)
			x[z.rd] = (uint64_t) z.imm16 << (16 * z.hw);
		else
			return 0;
	}
	return 1;
}

// --- tests ------------------------------------------------------------------

static uint32_t encode_and_not7(unsigned rd, unsigned rn)
{
	return MLDR_A64_AND_X_CLEAR_LOW3 | (rn << 5) | rd;
}

static void test_encodings(void)
{
	int ok = 1;
	for (unsigned r = 0; r < 32; r++) {
		// the constants describe the registers and bit patterns they claim
		struct mrs ro = decode_mrs(MLDR_A64_MRS_TPIDRRO_EL0 | r);
		struct mrs rw = decode_mrs(MLDR_A64_MRS_TPIDR_EL0 | r);
		ok &= ro.ok && ro.op0 == 3 && ro.op1 == 3 && ro.crn == 13 && ro.crm == 0 && ro.op2 == 3 && ro.rt == r;
		ok &= rw.ok && rw.op0 == 3 && rw.op1 == 3 && rw.crn == 13 && rw.crm == 0 && rw.op2 == 2 && rw.rt == r;
		struct and_imm a = decode_and_x(encode_and_not7(r, 31 - r));
		ok &= a.ok && a.rd == r && a.rn == 31 - r && a.imm == ~7ull;
		struct ldr_imm l = decode_ldr_x(MLDR_A64_LDR_X_UOFFSET | (8u << 10) | (r << 5) | (31 - r));
		ok &= l.ok && l.offset == 64 && l.rn == r && l.rt == 31 - r;
	}
	check(ok, "MRS TPIDRRO_EL0/TPIDR_EL0, AND #~7 and LDR constants decode to the documented fields");

	// the reference decoder itself: well-known logical immediates
	uint64_t v;
	check(decode_bitmask(1, 60, 61, &v) && v == ~7ull, "reference decoder: N=1 immr=61 imms=60 is ~7");
	check(decode_bitmask(1, 2, 0, &v) && v == 7, "reference decoder: N=1 immr=0 imms=2 is 7");
	check(decode_bitmask(0, 0x3C, 0, &v) && v == 0x5555555555555555ull, "reference decoder: N=0 imms=0x3c repeats 0b01");
	check(!decode_bitmask(1, 63, 0, &v), "reference decoder: an all-ones run is not encodable");
}

static void test_pair_detection(void)
{
	uint32_t reg;
	uint32_t mrs8 = MLDR_A64_MRS_TPIDRRO_EL0 | 8;
	check(mldr_tsd_read_pair(mrs8, encode_and_not7(8, 8), &reg) && reg == 8, "pair: mrs x8 + and x8, x8, #~7 matches");
	check(!mldr_tsd_read_pair(mrs8, encode_and_not7(9, 8), &reg), "pair: different destination does not match");
	check(!mldr_tsd_read_pair(mrs8, encode_and_not7(8, 9), &reg), "pair: different source does not match");
	// AND Xd, Xn, #7 (the CPU number), N=1 immr=0 imms=2
	uint32_t and7 = 0x92400000u | (0u << 16) | (2u << 10) | (8 << 5) | 8;
	check(!mldr_tsd_read_pair(mrs8, and7, &reg), "pair: and #7 (CPU number) does not match");
	check(!mldr_tsd_read_pair(mrs8, 0xD503201Fu /* nop */, &reg), "pair: lone mrs does not match");
	check(!mldr_tsd_read_pair(MLDR_A64_MRS_TPIDR_EL0 | 8, encode_and_not7(8, 8), &reg), "pair: mrs TPIDR_EL0 does not match");
	check(!mldr_tsd_read_pair(MLDR_A64_MRS_TPIDRRO_EL0 | 31, encode_and_not7(31, 31), &reg), "pair: XZR/SP never matches");
	// 32-bit AND (sf=0) with the same fields must not match
	check(!mldr_tsd_read_pair(mrs8, encode_and_not7(8, 8) & 0x7FFFFFFFu, &reg), "pair: 32-bit and does not match");
	// the registers named by the pair are reported for every register 0..30
	int ok = 1;
	for (uint32_t r = 0; r < 31; r++)
		ok &= mldr_tsd_read_pair(MLDR_A64_MRS_TPIDRRO_EL0 | r, encode_and_not7(r, r), &reg) && reg == r;
	check(ok, "pair: matches for x0..x30");
}

static void test_slot_offset(void)
{
	check(!mldr_tsd_slot_offset_valid(0) && !mldr_tsd_slot_offset_valid(8), "slot offset: below the thread control block is refused");
	check(mldr_tsd_slot_offset_valid(16) && mldr_tsd_slot_offset_valid(64) && mldr_tsd_slot_offset_valid(32760), "slot offset: aligned values in range accepted");
	check(!mldr_tsd_slot_offset_valid(32768) && !mldr_tsd_slot_offset_valid(65) && !mldr_tsd_slot_offset_valid((uintptr_t) -1), "slot offset: unaligned or beyond the LDR range refused");
}

static void test_rewrite(void)
{
	const uint32_t nop = 0xD503201Fu;
	uint32_t code[] = {
		nop,
		MLDR_A64_MRS_TPIDRRO_EL0 | 0, encode_and_not7(0, 0),            // pair, x0
		nop,
		MLDR_A64_MRS_TPIDRRO_EL0 | 9, 0x92400000u | (2u << 10) | (9 << 5) | 9, // CPU number: untouched
		MLDR_A64_MRS_TPIDRRO_EL0 | 30, encode_and_not7(30, 30),          // pair, x30
		MLDR_A64_MRS_TPIDRRO_EL0 | 5,                                     // pair split by the end of the buffer
	};
	uint32_t before[sizeof(code) / 4];
	memcpy(before, code, sizeof(code));
	size_t count = sizeof(code) / 4;

	check(mldr_tsd_count_reads(code, count) == 2, "rewrite: two pairs counted");
	check(mldr_tsd_rewrite_reads(code, count, 15) == 0 && memcmp(code, before, sizeof(code)) == 0, "rewrite: invalid slot offset changes nothing");
	check(mldr_tsd_rewrite_reads(code, count, 64) == 2, "rewrite: two pairs rewritten");

	int ok = 1;
	struct mrs m = decode_mrs(code[1]);
	struct ldr_imm l = decode_ldr_x(code[2]);
	ok &= m.ok && m.op2 == 2 && m.rt == 0 && l.ok && l.rn == 0 && l.rt == 0 && l.offset == 64;
	m = decode_mrs(code[6]);
	l = decode_ldr_x(code[7]);
	ok &= m.ok && m.op2 == 2 && m.rt == 30 && l.ok && l.rn == 30 && l.rt == 30 && l.offset == 64;
	check(ok, "rewrite: pairs become mrs TPIDR_EL0 + ldr [Xt, #slot]");

	check(code[0] == before[0] && code[3] == before[3] && code[4] == before[4] && code[5] == before[5] && code[8] == before[8],
		"rewrite: neighbours, the CPU number read and a trailing lone mrs are untouched");
	check(mldr_tsd_count_reads(code, count) == 0, "rewrite: no TPIDRRO_EL0 pairs remain");
	check(mldr_tsd_rewrite_reads(code, count, 64) == 0, "rewrite: idempotent");

	uint32_t big_slot[] = { MLDR_A64_MRS_TPIDRRO_EL0 | 1, encode_and_not7(1, 1) };
	mldr_tsd_rewrite_reads(big_slot, 2, 32760);
	l = decode_ldr_x(big_slot[1]);
	check(l.ok && l.offset == 32760, "rewrite: largest slot offset encodes exactly");

	check(mldr_tsd_rewrite_reads(NULL, 0, 64) == 0 && mldr_tsd_rewrite_reads(big_slot, 1, 64) == 0, "rewrite: empty and one-word buffers are fine");
}

static uint32_t encode_and_low12(int wide, unsigned rd, unsigned rn)
{
	return (wide ? MLDR_A64_AND_X_LOW12 : MLDR_A64_AND_W_LOW12) | (rn << 5) | rd;
}

static void test_cpu_encodings(void)
{
	int ok = 1;
	for (unsigned r = 0; r < 31; r++)
	{
		struct and_imm a = decode_and_x(encode_and_low12(1, r, 30 - r));
		ok &= a.ok && a.rd == r && a.rn == 30 - r && a.imm == 0xfff;
		a = decode_and_w(encode_and_low12(0, r, 30 - r));
		ok &= a.ok && a.rd == r && a.rn == 30 - r && a.imm == 0xfff;
	}
	struct ldr_w l = decode_ldr_w(MLDR_A64_LDR_W_UOFFSET | (4095u << 10) | (8u << 5) | 0);
	ok &= l.ok && l.offset == 16380 && l.rn == 8 && l.rt == 0;
	l = decode_ldr_w(MLDR_A64_LDUR_W | (0x100u << 12) | (8u << 5) | 1);
	ok &= l.ok && l.offset == -256 && l.rn == 8 && l.rt == 1;
	struct movz z = decode_movz(MLDR_A64_MOVZ_W | 5);
	ok &= z.ok && !z.sf && z.hw == 0 && z.imm16 == 0 && z.rd == 5;
	check(ok, "AND #0xfff (W and X), LDR/LDUR (32-bit) and MOVZ constants decode to the documented fields");
	// the same words the LLVM assembler produces
	check(encode_and_low12(1, 8, 8) == 0x92402D08u && encode_and_low12(0, 0, 8) == 0x12002D00u,
		"and x8, x8, #0xfff and and w0, w8, #0xfff match the assembler");
	struct mldr_cpu_number_source src = mldr_cpu_number_source_for(true, 16380);
	check(mldr_cpu_number_load(src, 8, 0) == 0xB97FFD00u, "ldr w0, [x8, #16380] matches the assembler");
	src = mldr_cpu_number_source_for(true, -256);
	check(mldr_cpu_number_load(src, 8, 0) == 0xB8500100u, "ldur w0, [x8, #-256] matches the assembler");
	src = mldr_cpu_number_source_for(true, -32);
	check(mldr_cpu_number_load(src, 8, 1) == 0xB85E0101u, "ldur w1, [x8, #-32] matches the assembler");
	src = mldr_cpu_number_source_for(false, 0);
	check(mldr_cpu_number_load(src, 8, 5) == 0x52800005u, "mov w5, #0 matches the assembler");
}

static void test_cpu_source(void)
{
	struct mldr_cpu_number_source s;
	s = mldr_cpu_number_source_for(false, 64);
	check(!s.rseq, "cpu source: no rseq area gives the constant");
	s = mldr_cpu_number_source_for(true, 0);
	check(s.rseq && s.offset == 0, "cpu source: offset 0 loads");
	s = mldr_cpu_number_source_for(true, 16380);
	check(s.rseq, "cpu source: largest LDR offset loads");
	s = mldr_cpu_number_source_for(true, 16384);
	check(!s.rseq, "cpu source: beyond LDR range gives the constant");
	s = mldr_cpu_number_source_for(true, -256);
	check(s.rseq, "cpu source: smallest LDUR offset loads");
	s = mldr_cpu_number_source_for(true, -260);
	check(!s.rseq, "cpu source: below LDUR range gives the constant");
	s = mldr_cpu_number_source_for(true, -30);
	check(!s.rseq, "cpu source: misaligned offset gives the constant");
}

static void test_cpu_pair_detection(void)
{
	uint32_t rd;
	uint32_t mrs8 = MLDR_A64_MRS_TPIDR_EL0 | 8;
	check(mldr_cpu_read_pair(mrs8, encode_and_low12(1, 8, 8), &rd) && rd == 8, "cpu pair: mrs x8 + and x8, x8, #0xfff matches");
	check(mldr_cpu_read_pair(mrs8, encode_and_low12(0, 0, 8), &rd) && rd == 0, "cpu pair: and w0, w8, #0xfff matches with destination w0");
	check(!mldr_cpu_read_pair(mrs8, encode_and_low12(1, 8, 9), &rd), "cpu pair: AND of another register does not match");
	check(!mldr_cpu_read_pair(mrs8, encode_and_low12(1, 31, 8), &rd), "cpu pair: AND into SP does not match");
	check(!mldr_cpu_read_pair(MLDR_A64_MRS_TPIDR_EL0 | 31, encode_and_low12(1, 0, 31), &rd), "cpu pair: MRS into XZR does not match");
	check(!mldr_cpu_read_pair(MLDR_A64_MRS_TPIDRRO_EL0 | 8, encode_and_low12(1, 8, 8), &rd), "cpu pair: TPIDRRO_EL0 does not match");
	check(!mldr_cpu_read_pair(mrs8, encode_and_not7(8, 8), &rd), "cpu pair: and #~7 does not match");
	check(!mldr_cpu_read_pair(mrs8, 0x92400000u | (2u << 10) | (8 << 5) | 8, &rd), "cpu pair: and #7 does not match");
	check(!mldr_cpu_read_pair(mrs8, 0x92402800u | (8 << 5) | 8, &rd), "cpu pair: and #0x7ff does not match");
	// a native thread pointer use (what the TSD rewrite produces) is not a CPU number read
	check(!mldr_cpu_read_pair(mrs8, MLDR_A64_LDR_X_UOFFSET | (8u << 10) | (8 << 5) | 8, &rd), "cpu pair: mrs TPIDR_EL0 + ldr does not match");
	check(!mldr_cpu_read_pair(mrs8, 0xD503201Fu, &rd), "cpu pair: lone mrs does not match");
	check(!mldr_tsd_read_pair(mrs8, encode_and_low12(1, 8, 8), &rd), "tsd pair: a CPU number read is not a TSD base read");
}

static void test_cpu_rewrite(void)
{
	const uint32_t nop = 0xD503201Fu;
	uint32_t code[] = {
		nop,
		MLDR_A64_MRS_TPIDR_EL0 | 8, encode_and_low12(0, 0, 8),          // w0 = cpu
		MLDR_A64_MRS_TPIDRRO_EL0 | 3, encode_and_not7(3, 3),             // TSD base pair
		MLDR_A64_MRS_TPIDR_EL0 | 9, encode_and_low12(1, 9, 9),          // x9 = cpu
		MLDR_A64_MRS_TPIDR_EL0 | 10, MLDR_A64_LDR_X_UOFFSET | (10 << 5) | 10, // native TLS use
		MLDR_A64_MRS_TPIDR_EL0 | 11,                                     // split by the end
	};
	const size_t count = sizeof(code) / 4;
	uint32_t before[sizeof(code) / 4];
	memcpy(before, code, sizeof(code));

	check(mldr_cpu_count_reads(code, count) == 2, "cpu rewrite: two pairs counted");
	check(mldr_tsd_count_reads(code, count) == 1, "cpu rewrite: the TSD pair is counted separately");

	// the same order as the loader: TSD pairs, then CPU number pairs
	mldr_tsd_rewrite_reads(code, count, 64);
	check(mldr_cpu_count_reads(code, count) == 2, "cpu rewrite: the TSD rewrite creates no CPU number pair");
	check(mldr_cpu_rewrite_reads(code, count, mldr_cpu_number_source_for(true, -32)) == 2, "cpu rewrite: two pairs rewritten");
	check(code[1] == before[1] && code[5] == before[5], "cpu rewrite: the MRS words stay");
	check(code[0] == before[0] && code[7] == before[7] && code[8] == before[8] && code[9] == before[9],
		"cpu rewrite: neighbours, the native TLS use and a trailing lone mrs are untouched");
	check(mldr_cpu_count_reads(code, count) == 0 && mldr_cpu_rewrite_reads(code, count, mldr_cpu_number_source_for(true, 0)) == 0,
		"cpu rewrite: idempotent");
	check(mldr_cpu_rewrite_reads(NULL, 0, mldr_cpu_number_source_for(true, 0)) == 0, "cpu rewrite: empty buffer is fine");
}

// Runs rewritten pairs on the model: the result is the CPU number from the
// rseq area (or 0), the MRS register keeps the thread pointer.
static void test_cpu_semantics(void)
{
	const uint64_t tp = 0x7f0000001000ull;
	uint32_t memory[8192];
	const uint64_t mem_base = tp - 1024;
	memset(memory, 0xEE, sizeof(memory));
	const intptr_t offsets[] = { -256, -64, -32, 0, 4, 16380 };
	int ok = 1;
	for (size_t o = 0; o < sizeof(offsets) / sizeof(offsets[0]); o++)
	{
		memory[(tp + offsets[o] - mem_base) / 4] = 37; // cpu_id_start
		for (int wide = 0; wide < 2; wide++)
		for (unsigned t = 0; t < 31; t += 5)
		for (unsigned d = 0; d < 31; d += 6)
		{
			uint32_t pair[2] = { MLDR_A64_MRS_TPIDR_EL0 | t, encode_and_low12(wide, d, t) };
			uint64_t x[32];
			for (int i = 0; i < 32; i++)
				x[i] = 0x1111111111111111ull * (i & 15);
			ok &= mldr_cpu_rewrite_reads(pair, 2, mldr_cpu_number_source_for(true, offsets[o])) == 1;
			ok &= run_cpu_pair(pair, tp, memory, mem_base, 8192, x);
			ok &= x[d] == 37;
			if (d != t)
				ok &= x[t] == tp;
		}
		memory[(tp + offsets[o] - mem_base) / 4] = 0xEEEEEEEEu;
	}
	check(ok, "cpu semantics: rewritten pairs yield the rseq CPU number, zero-extended, for W/X and any registers");

	ok = 1;
	for (unsigned d = 0; d < 31; d++)
	{
		uint32_t pair[2] = { MLDR_A64_MRS_TPIDR_EL0 | 4, encode_and_low12(1, d, 4) };
		uint64_t x[32];
		memset(x, 0xAB, sizeof(x));
		mldr_cpu_rewrite_reads(pair, 2, mldr_cpu_number_source_for(false, 0));
		ok &= run_cpu_pair(pair, tp, memory, mem_base, 8192, x) && x[d] == 0;
	}
	check(ok, "cpu semantics: without an rseq area the result is CPU 0");

	uint32_t orig[2] = { MLDR_A64_MRS_TPIDR_EL0 | 8, encode_and_low12(0, 8, 8) };
	uint64_t x[32] = {0};
	check(run_cpu_pair(orig, tp, memory, mem_base, 8192, x) && x[8] == (tp & 0xfff),
		"cpu semantics: model check, the original pair yields TP & 0xfff");
}

int main(void)
{
	test_encodings();
	test_pair_detection();
	test_slot_offset();
	test_rewrite();
	test_cpu_encodings();
	test_cpu_source();
	test_cpu_pair_detection();
	test_cpu_rewrite();
	test_cpu_semantics();
	if (failures) {
		printf("FAIL: %d checks failed\n", failures);
		return 1;
	}
	puts("PASS tsd_rewrite");
	return 0;
}
