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

/*
 * The matrix_identity_<type>NxN constants declared by <simd/matrix_types.h>.
 * Applications import them from libSystem (which re-exports libsystem_m).
 */

/* The matrix layouts of <simd/vector_types.h>, repeated so that the math
 * library does not depend on the SDK headers. */
typedef float identity_float2 __attribute__((ext_vector_type(2)));
typedef float identity_float3 __attribute__((ext_vector_type(3)));
typedef float identity_float4 __attribute__((ext_vector_type(4)));
typedef double identity_double2 __attribute__((ext_vector_type(2)));
typedef double identity_double3 __attribute__((ext_vector_type(3)));
typedef double identity_double4 __attribute__((ext_vector_type(4)));

typedef struct { identity_float2 columns[2]; } identity_float2x2;
typedef struct { identity_float3 columns[3]; } identity_float3x3;
typedef struct { identity_float4 columns[4]; } identity_float4x4;
typedef struct { identity_double2 columns[2]; } identity_double2x2;
typedef struct { identity_double3 columns[3]; } identity_double3x3;
typedef struct { identity_double4 columns[4]; } identity_double4x4;

const identity_float2x2 matrix_identity_float2x2 = {{{1, 0}, {0, 1}}};
const identity_float3x3 matrix_identity_float3x3 = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
const identity_float4x4 matrix_identity_float4x4 = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
const identity_double2x2 matrix_identity_double2x2 = {{{1, 0}, {0, 1}}};
const identity_double3x3 matrix_identity_double3x3 = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
const identity_double4x4 matrix_identity_double4x4 = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
