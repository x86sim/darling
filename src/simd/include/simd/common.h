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

#ifndef SIMD_COMMON_H
#define SIMD_COMMON_H

#include <simd/base.h>
#include <simd/vector_types.h>

/*
 * Element-wise functions on scalars and 2-, 3- and 4-element vectors:
 * abs, min, max, clamp, sign, recip, rsqrt, fract, step, smoothstep, mix and
 * the horizontal reductions reduce_add, reduce_min and reduce_max.
 */

#define SIMD_SUM2(v) ((v).x + (v).y)
#define SIMD_SUM3(v) ((v).x + (v).y + (v).z)
#define SIMD_SUM4(v) (((v).x + (v).y) + ((v).z + (v).w))
#define SIMD_MIN2(v, m) m((v).x, (v).y)
#define SIMD_MIN3(v, m) m(m((v).x, (v).y), (v).z)
#define SIMD_MIN4(v, m) m(m((v).x, (v).y), m((v).z, (v).w))

/* Selects between the two element-wise scalar helpers. */
#define SIMD_SMIN(a, b) ((b) < (a) ? (b) : (a))
#define SIMD_SMAX(a, b) ((a) < (b) ? (b) : (a))

/* Integer and floating-point vectors: min, max, clamp and reductions. */
#define SIMD_COMMON_ANY(V, S, N)                                              \
SIMD_FN V simd_min(V x, V y) { return __builtin_elementwise_min(x, y); }      \
SIMD_FN V simd_max(V x, V y) { return __builtin_elementwise_max(x, y); }      \
SIMD_FN V simd_clamp(V x, V lo, V hi) {                                       \
    return __builtin_elementwise_min(__builtin_elementwise_max(x, lo), hi);   \
}                                                                             \
SIMD_FN S simd_reduce_add(V x) { return SIMD_SUM##N(x); }                     \
SIMD_FN S simd_reduce_min(V x) { return SIMD_MIN##N(x, SIMD_SMIN); }          \
SIMD_FN S simd_reduce_max(V x) { return SIMD_MIN##N(x, SIMD_SMAX); }

/* Floating-point vectors. */
#define SIMD_COMMON_FLOAT(V, S, N)                                      \
SIMD_COMMON_ANY(V, S, N)                                                      \
SIMD_FN V simd_abs(V x) { return __builtin_elementwise_abs(x); }              \
SIMD_FN V simd_sign(V x) {                                                    \
    return __builtin_convertvector((x < (S)0), V)                             \
         - __builtin_convertvector((x > (S)0), V);                            \
}                                                                             \
SIMD_FN V simd_recip(V x) { return (S)1 / x; }                                \
SIMD_FN V simd_rsqrt(V x) { return (S)1 / __builtin_elementwise_sqrt(x); }    \
SIMD_FN V simd_step(V edge, V x) {                                            \
    return (S)1 + __builtin_convertvector((x < edge), V);                     \
}                                                                             \
SIMD_FN V simd_fract(V x) {                                                   \
    V f = x - __builtin_elementwise_floor(x);                                 \
    return __builtin_elementwise_min(f, (V)(S)SIMD_BELOW_ONE_##S);            \
}                                                                             \
SIMD_FN V simd_mix(V x, V y, V t) { return x + (y - x) * t; }                 \
SIMD_FN V simd_smoothstep(V e0, V e1, V x) {                                  \
    V t = simd_clamp((x - e0) / (e1 - e0), (V)(S)0, (V)(S)1);                 \
    return t * t * ((S)3 - (S)2 * t);                                         \
}

/* Largest value below one, so that fract never returns 1. */
#define SIMD_BELOW_ONE_float 0x1.fffffep-1f
#define SIMD_BELOW_ONE_double 0x1.fffffffffffffp-1

SIMD_COMMON_FLOAT(simd_float2, float, 2)
SIMD_COMMON_FLOAT(simd_float3, float, 3)
SIMD_COMMON_FLOAT(simd_float4, float, 4)
SIMD_COMMON_FLOAT(simd_double2, double, 2)
SIMD_COMMON_FLOAT(simd_double3, double, 3)
SIMD_COMMON_FLOAT(simd_double4, double, 4)

SIMD_COMMON_ANY(simd_int2, int, 2)
SIMD_COMMON_ANY(simd_int3, int, 3)
SIMD_COMMON_ANY(simd_int4, int, 4)
SIMD_COMMON_ANY(simd_uint2, unsigned int, 2)
SIMD_COMMON_ANY(simd_uint3, unsigned int, 3)
SIMD_COMMON_ANY(simd_uint4, unsigned int, 4)

SIMD_FN simd_int2 simd_abs(simd_int2 x) { return __builtin_elementwise_abs(x); }
SIMD_FN simd_int3 simd_abs(simd_int3 x) { return __builtin_elementwise_abs(x); }
SIMD_FN simd_int4 simd_abs(simd_int4 x) { return __builtin_elementwise_abs(x); }

/* Scalar forms. */
#define SIMD_COMMON_SCALAR(S)                                                 \
SIMD_FN S simd_abs(S x) { return x < (S)0 ? -x : x; }                         \
SIMD_FN S simd_min(S x, S y) { return SIMD_SMIN(x, y); }                      \
SIMD_FN S simd_max(S x, S y) { return SIMD_SMAX(x, y); }                      \
SIMD_FN S simd_clamp(S x, S lo, S hi) {                                       \
    return simd_min(simd_max(x, lo), hi);                                     \
}                                                                             \
SIMD_FN S simd_sign(S x) { return x > (S)0 ? (S)1 : (x < (S)0 ? (S)-1 : (S)0); }\
SIMD_FN S simd_recip(S x) { return (S)1 / x; }                                \
SIMD_FN S simd_rsqrt(S x) { return (S)1 / __builtin_elementwise_sqrt(x); }    \
SIMD_FN S simd_step(S edge, S x) { return x < edge ? (S)0 : (S)1; }           \
SIMD_FN S simd_fract(S x) {                                                   \
    return simd_min(x - __builtin_elementwise_floor(x), SIMD_BELOW_ONE_##S);  \
}                                                                             \
SIMD_FN S simd_mix(S x, S y, S t) { return x + (y - x) * t; }                 \
SIMD_FN S simd_smoothstep(S e0, S e1, S x) {                                  \
    S t = simd_clamp((x - e0) / (e1 - e0), (S)0, (S)1);                       \
    return t * t * ((S)3 - (S)2 * t);                                         \
}

SIMD_COMMON_SCALAR(float)
SIMD_COMMON_SCALAR(double)
#undef SIMD_COMMON_SCALAR
#undef SIMD_COMMON_FLOAT
#undef SIMD_COMMON_ANY

#endif
