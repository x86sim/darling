/*
This file is part of Darling.

Copyright (C) 2026 Lubos Dolezel

Darling is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Darling is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Darling.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MLDR_FAT_SLICE_H_
#define _MLDR_FAT_SLICE_H_

#include <stdint.h>
#include <stddef.h>

// A universal ("fat") file with 32-bit (FAT_MAGIC) or 64-bit (FAT_MAGIC_64)
// architecture records, in either byte order.
#define MLDR_FAT_MAX_ARCHES 64
// The most bytes of the file mldr_fat_select() ever looks at: the header and
// the largest architecture table.
#define MLDR_FAT_MAX_HEADER (8 + MLDR_FAT_MAX_ARCHES * 32)

struct mldr_fat_slice
{
	uint64_t offset;
	uint64_t size;
	uint32_t cputype;
	uint32_t cpusubtype;
};

enum mldr_fat_result
{
	MLDR_FAT_OK = 0,
	MLDR_FAT_NOT_FAT,     // the magic is not a universal file's
	MLDR_FAT_MALFORMED,   // header or architecture table does not fit the file, or too many architectures
	MLDR_FAT_NO_MATCH,    // no usable slice for the requested/native architectures
};

/**
 * Whether the first four bytes of a file are the magic of a universal file
 * (any of the four spellings).
 */
int mldr_fat_is_magic(const uint8_t* bytes4);

/**
 * Picks the slice to run from a universal file. `data` holds the first `len`
 * bytes of the file (at most MLDR_FAT_MAX_HEADER are used), `file_size` is its
 * real size. Every architecture record is bounds-checked against the file; a
 * record whose slice is empty or lies (even partly) outside the file is never
 * selected. Nothing is allocated and `out` is only written on success.
 *
 * Selection: with `forced` nonzero the last slice of that cputype; otherwise
 * the slice matching the earliest nonzero entry of `prefs` (the first one among
 * equals); otherwise the last slice of `native`, else the first slice of
 * `fallback` (nonzero only where the host can run a second ABI).
 */
enum mldr_fat_result mldr_fat_select(const uint8_t* data, size_t len, uint64_t file_size,
		const uint32_t prefs[4], uint32_t forced, uint32_t native, uint32_t fallback,
		struct mldr_fat_slice* out);

/**
 * The CPU type a Mach-O file can run as: `cputype` if the file is a thin
 * Mach-O of that type or a universal file with a usable slice of it (bounds
 * checked like mldr_fat_select). `data` holds the first `len` bytes.
 */
int mldr_macho_has_arch(const uint8_t* data, size_t len, uint64_t file_size, uint32_t cputype);

/**
 * x86Sim: whether an x86_64 host runs a main executable as arm64 (under the
 * arm64 translator) rather than natively. `exe` is the start of the executable.
 * `sim` is nonzero for a simulator process (DYLD_ROOT_PATH is set); `dyld_sim`
 * is then the start of the runtime's dyld_sim (NULL/0 if it could not be read).
 *
 * A file without an arm64 slice runs natively, one with arm64 and no x86
 * slice as arm64. A universal file with both (an app built with
 * ONLY_ACTIVE_ARCH=NO) runs as arm64 in a simulator process whose runtime
 * cannot load x86_64 programs (dyld_sim has no x86_64 slice, or is unreadable:
 * every runtime x86Sim ships is arm64-only), as Apple's simulator does on an
 * arm64 Mac: its x86_64 slice would load an x86_64 dyld that refuses the arm64
 * dyld_sim ("dyld_sim is not compatible with the loaded process", code 9).
 * Otherwise (Darling's own programs, an x86_64-capable runtime) the binary
 * preferences `prefs` (posix_spawnattr_setbinpref) pick, else x86_64.
 */
int mldr_runs_as_arm64(const uint8_t* exe, size_t exe_len, uint64_t exe_size, const uint32_t prefs[4],
		int sim, const uint8_t* dyld_sim, size_t dyld_sim_len, uint64_t dyld_sim_size);

#endif
