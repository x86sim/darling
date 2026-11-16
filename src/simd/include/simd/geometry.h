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

#ifndef SIMD_GEOMETRY_H
#define SIMD_GEOMETRY_H

#include <simd/common.h>

/*
 * Geometric functions on floating-point vectors: dot, length,
 * length_squared, distance, distance_squared, normalize, cross, project,
 * reflect and refract.
 */

#define SIMD_GEOMETRY(V, S, N)                                                \
SIMD_FN S simd_dot(V x, V y) { return simd_reduce_add(x * y); }               \
SIMD_FN S simd_length_squared(V x) { return simd_dot(x, x); }                 \
SIMD_FN S simd_length(V x) { return __builtin_elementwise_sqrt(simd_dot(x, x)); }\
SIMD_FN S simd_distance_squared(V x, V y) { return simd_length_squared(x - y); }\
SIMD_FN S simd_distance(V x, V y) { return simd_length(x - y); }              \
SIMD_FN V simd_normalize(V x) { return x * ((S)1 / simd_length(x)); }         \
SIMD_FN V simd_project(V x, V y) { return y * (simd_dot(x, y) / simd_dot(y, y)); }\
SIMD_FN V simd_reflect(V x, V n) { return x - (S)2 * simd_dot(x, n) * n; }    \
SIMD_FN V simd_refract(V x, V n, S eta) {                                     \
    S d = simd_dot(x, n);                                                     \
    S k = (S)1 - eta * eta * ((S)1 - d * d);                                  \
    if (k < (S)0) return (V)(S)0;                                             \
    return eta * x - (eta * d + __builtin_elementwise_sqrt(k)) * n;           \
}

SIMD_GEOMETRY(simd_float2, float, 2)
SIMD_GEOMETRY(simd_float3, float, 3)
SIMD_GEOMETRY(simd_float4, float, 4)
SIMD_GEOMETRY(simd_double2, double, 2)
SIMD_GEOMETRY(simd_double3, double, 3)
SIMD_GEOMETRY(simd_double4, double, 4)
#undef SIMD_GEOMETRY

/* The cross product of 2-element vectors is the scalar z component. */
SIMD_FN float simd_cross(simd_float2 x, simd_float2 y) { return x.x * y.y - x.y * y.x; }
SIMD_FN double simd_cross(simd_double2 x, simd_double2 y) { return x.x * y.y - x.y * y.x; }
SIMD_FN simd_float3 simd_cross(simd_float3 x, simd_float3 y) {
    return (simd_float3){x.y * y.z - x.z * y.y, x.z * y.x - x.x * y.z, x.x * y.y - x.y * y.x};
}
SIMD_FN simd_double3 simd_cross(simd_double3 x, simd_double3 y) {
    return (simd_double3){x.y * y.z - x.z * y.y, x.z * y.x - x.x * y.z, x.x * y.y - x.y * y.x};
}

#endif
