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

#ifndef SIMD_EXTERN_H
#define SIMD_EXTERN_H

#include <simd/vector_types.h>

/*
 * Entry points of the system math library that the inline matrix functions
 * call. They take and return the matrix by value.
 */
#ifdef __cplusplus
extern "C" {
#endif

extern simd_float2x2 __invert_f2(simd_float2x2);
extern simd_float3x3 __invert_f3(simd_float3x3);
extern simd_float4x4 __invert_f4(simd_float4x4);
extern simd_double2x2 __invert_d2(simd_double2x2);
extern simd_double3x3 __invert_d3(simd_double3x3);
extern simd_double4x4 __invert_d4(simd_double4x4);

#ifdef __cplusplus
}
#endif

#endif
