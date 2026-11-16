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

#ifndef SIMD_MATRIX_TYPES_H
#define SIMD_MATRIX_TYPES_H

#include <simd/vector_types.h>

/*
 * matrix_<type>CxR aliases of the simd_<type>CxR matrix types, which have C
 * columns of R elements each, and the identity matrices of the system math
 * library.
 */

typedef simd_float2x2 matrix_float2x2;
typedef simd_float3x2 matrix_float3x2;
typedef simd_float4x2 matrix_float4x2;
typedef simd_float2x3 matrix_float2x3;
typedef simd_float3x3 matrix_float3x3;
typedef simd_float4x3 matrix_float4x3;
typedef simd_float2x4 matrix_float2x4;
typedef simd_float3x4 matrix_float3x4;
typedef simd_float4x4 matrix_float4x4;

typedef simd_double2x2 matrix_double2x2;
typedef simd_double3x2 matrix_double3x2;
typedef simd_double4x2 matrix_double4x2;
typedef simd_double2x3 matrix_double2x3;
typedef simd_double3x3 matrix_double3x3;
typedef simd_double4x3 matrix_double4x3;
typedef simd_double2x4 matrix_double2x4;
typedef simd_double3x4 matrix_double3x4;
typedef simd_double4x4 matrix_double4x4;

#ifdef __cplusplus
extern "C" {
#endif

extern const simd_float2x2 matrix_identity_float2x2;
extern const simd_float3x3 matrix_identity_float3x3;
extern const simd_float4x4 matrix_identity_float4x4;
extern const simd_double2x2 matrix_identity_double2x2;
extern const simd_double3x3 matrix_identity_double3x3;
extern const simd_double4x4 matrix_identity_double4x4;

#ifdef __cplusplus
}
#endif

#endif
