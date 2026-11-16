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

#ifndef SIMD_QUATERNION_H
#define SIMD_QUATERNION_H

#include <simd/matrix.h>

/*
 * Quaternion functions on simd_quatf and simd_quatd. A quaternion keeps its
 * imaginary part in the x, y and z elements of `vector` and its real part in
 * w. Rotation functions expect unit quaternions.
 */

#define SIMD_QUATERNION(Q, T, V3, V4, M3, M4, SQRT, ATAN2, SIN, COS, EXP, LOG, ACOS) \
SIMD_FN Q simd_quaternion(T ix, T iy, T iz, T r) {                            \
    Q q = {{ix, iy, iz, r}};                                                  \
    return q;                                                                 \
}                                                                             \
SIMD_FN Q simd_quaternion(V4 v) { Q q = {v}; return q; }                      \
SIMD_FN Q simd_quaternion(T angle, V3 axis) {                                 \
    V3 i = axis * SIN(angle / 2);                                             \
    Q q = {{i.x, i.y, i.z, COS(angle / 2)}};                                  \
    return q;                                                                 \
}                                                                             \
SIMD_FN T simd_length(Q q) { return simd_length(q.vector); }                  \
SIMD_FN T simd_dot(Q a, Q b) { return simd_dot(a.vector, b.vector); }         \
SIMD_FN bool simd_equal(Q a, Q b) {                                          \
    return a.vector.x == b.vector.x && a.vector.y == b.vector.y &&            \
           a.vector.z == b.vector.z && a.vector.w == b.vector.w;              \
}                                                                             \
SIMD_FN V3 simd_imag(Q q) { return q.vector.xyz; }                            \
SIMD_FN T simd_real(Q q) { return q.vector.w; }                               \
SIMD_FN Q simd_add(Q a, Q b) { Q q = {a.vector + b.vector}; return q; }       \
SIMD_FN Q simd_sub(Q a, Q b) { Q q = {a.vector - b.vector}; return q; }      \
SIMD_FN Q simd_negate(Q a) { Q q = {-a.vector}; return q; }                   \
SIMD_FN Q simd_mul(T s, Q a) { Q q = {s * a.vector}; return q; }              \
SIMD_FN Q simd_mul(Q a, T s) { Q q = {a.vector * s}; return q; }              \
SIMD_FN Q simd_conjugate(Q a) {                                               \
    Q q = {{-a.vector.x, -a.vector.y, -a.vector.z, a.vector.w}};              \
    return q;                                                                 \
}                                                                             \
SIMD_FN Q simd_inverse(Q a) {                                                 \
    Q c = simd_conjugate(a);                                                  \
    return simd_mul(1 / simd_dot(a, a), c);                                   \
}                                                                             \
SIMD_FN Q simd_mul(Q a, Q b) {                                                \
    V3 ai = a.vector.xyz, bi = b.vector.xyz;                                  \
    T aw = a.vector.w, bw = b.vector.w;                                       \
    V3 i = aw * bi + bw * ai + simd_cross(ai, bi);                            \
    Q q = {{i.x, i.y, i.z, aw * bw - simd_dot(ai, bi)}};                      \
    return q;                                                                 \
}                                                                             \
/* QUIRK: a zero quaternion normalizes to the identity (0, 0, 0, 1) rather   \
 * than to NaN. Not documented. Evidence: black-box observation on macOS     \
 * 26.7.3 x86_64 (tier 2), vd-upstream MAC-EVIDENCE.md. Test:                \
 * test_quirk_zero_quaternion_normalizes_to_identity in simd_math.c. */       \
SIMD_FN Q simd_normalize(Q a) {                                               \
    T len = simd_length(a);                                                   \
    Q identity = {{0, 0, 0, 1}};                                              \
    if (len == 0) return identity;                                            \
    return simd_mul(1 / len, a);                                              \
}                                                                             \
SIMD_FN T simd_angle(Q q) {                                                   \
    return 2 * ATAN2(simd_length(q.vector.xyz), q.vector.w);                  \
}                                                                             \
/* The identity rotation has no axis: the division yields NaN elements, as   \
 * observed on macOS 26.7.3 (MAC-EVIDENCE.md); test_quirk_identity_axis_is_nan.*/ \
SIMD_FN V3 simd_axis(Q q) {                                                   \
    V3 i = q.vector.xyz;                                                      \
    return i / simd_length(i);                                                \
}                                                                             \
SIMD_FN V3 simd_act(Q q, V3 v) {                                              \
    V3 i = q.vector.xyz;                                                      \
    V3 t = 2 * simd_cross(i, v);                                              \
    return v + q.vector.w * t + simd_cross(i, t);                             \
}                                                                             \
SIMD_FN M3 simd_matrix3x3(Q q) {                                              \
    T x = q.vector.x, y = q.vector.y, z = q.vector.z, w = q.vector.w;         \
    M3 m = {{                                                                 \
        (V3){1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w)},\
        (V3){2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w)},\
        (V3){2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y)}}};\
    return m;                                                                 \
}                                                                             \
SIMD_FN M4 simd_matrix4x4(Q q) {                                              \
    M3 r = simd_matrix3x3(q);                                                 \
    M4 m = {{                                                                 \
        (V4){r.columns[0].x, r.columns[0].y, r.columns[0].z, 0},              \
        (V4){r.columns[1].x, r.columns[1].y, r.columns[1].z, 0},              \
        (V4){r.columns[2].x, r.columns[2].y, r.columns[2].z, 0},              \
        (V4){0, 0, 0, 1}}};                                                   \
    return m;                                                                 \
}                                                                             \
/* The columns of the rotation matrix are the images of the basis vectors. */ \
SIMD_FN Q simd_quaternion(M3 m) {                                             \
    T m00 = m.columns[0].x, m10 = m.columns[0].y, m20 = m.columns[0].z;       \
    T m01 = m.columns[1].x, m11 = m.columns[1].y, m21 = m.columns[1].z;       \
    T m02 = m.columns[2].x, m12 = m.columns[2].y, m22 = m.columns[2].z;       \
    T tr = m00 + m11 + m22, x, y, z, w, s;                                    \
    if (tr > 0) {                                                             \
        s = 2 * SQRT(tr + 1);                                                 \
        w = s / 4; x = (m21 - m12) / s; y = (m02 - m20) / s; z = (m10 - m01) / s;\
    } else if (m00 > m11 && m00 > m22) {                                      \
        s = 2 * SQRT(1 + m00 - m11 - m22);                                    \
        w = (m21 - m12) / s; x = s / 4; y = (m01 + m10) / s; z = (m02 + m20) / s;\
    } else if (m11 > m22) {                                                   \
        s = 2 * SQRT(1 + m11 - m00 - m22);                                    \
        w = (m02 - m20) / s; x = (m01 + m10) / s; y = s / 4; z = (m12 + m21) / s;\
    } else {                                                                  \
        s = 2 * SQRT(1 + m22 - m00 - m11);                                    \
        w = (m10 - m01) / s; x = (m02 + m20) / s; y = (m12 + m21) / s; z = s / 4;\
    }                                                                         \
    Q q = {{x, y, z, w}};                                                     \
    return q;                                                                 \
}                                                                             \
SIMD_FN Q simd_quaternion(M4 m) {                                             \
    M3 r = {{m.columns[0].xyz, m.columns[1].xyz, m.columns[2].xyz}};          \
    return simd_quaternion(r);                                                \
}                                                                             \
/* The shortest rotation from one unit vector to another. */                  \
/* QUIRK: for exactly opposite vectors the rotation is a half turn about     \
 * normalize(cross(from, e)), where e is the basis vector along the smallest \
 * |component| of from (the first one on ties). Not documented. Evidence:    \
 * black-box observation on macOS 26.7.3 x86_64 (tier 2) for six inputs,     \
 * MAC-EVIDENCE.md. Test: test_quirk_opposite_vectors_rotation_axis. */      \
SIMD_FN Q simd_quaternion(V3 from, V3 to) {                                   \
    T d = simd_dot(from, to);                                                 \
    V3 axis = simd_cross(from, to);                                           \
    if (d < -1 + 1e-6) {                                                      \
        /* Opposite vectors: rotate by pi about any perpendicular axis. */    \
        T ax = simd_abs(from.x), ay = simd_abs(from.y), az = simd_abs(from.z);\
        V3 other = (V3){1, 0, 0};                                             \
        if (ay < ax && ay <= az) other = (V3){0, 1, 0};                       \
        else if (az < ax && az < ay) other = (V3){0, 0, 1};                   \
        axis = simd_normalize(simd_cross(from, other));                       \
        Q q = {{axis.x, axis.y, axis.z, 0}};                                  \
        return q;                                                             \
    }                                                                         \
    Q q = {{axis.x, axis.y, axis.z, 1 + d}};                                  \
    return simd_normalize(q);                                                 \
}                                                                             \
SIMD_FN Q simd_exp(Q q) {                                                     \
    V3 i = q.vector.xyz;                                                      \
    T len = simd_length(i), e = EXP(q.vector.w);                              \
    T k = len == 0 ? 0 : e * SIN(len) / len;                                  \
    Q r = {{k * i.x, k * i.y, k * i.z, e * COS(len)}};                        \
    return r;                                                                 \
}                                                                             \
SIMD_FN Q simd_log(Q q) {                                                     \
    V3 i = q.vector.xyz;                                                      \
    T len = simd_length(i), norm = simd_length(q);                            \
    T k = len == 0 ? 0 : ACOS(q.vector.w / norm) / len;                       \
    Q r = {{k * i.x, k * i.y, k * i.z, LOG(norm)}};                           \
    return r;                                                                 \
}                                                                             \
/* The names the C tgmath conventions use for the generic exp and log. */     \
SIMD_FN Q __tg_exp(Q q) { return simd_exp(q); }                               \
SIMD_FN Q __tg_log(Q q) { return simd_log(q); }                               \
SIMD_FN Q simd_slerp(Q a, Q b, T t) {                                         \
    T d = simd_dot(a, b);                                                     \
    Q to = d < 0 ? simd_negate(b) : b;                                        \
    d = d < 0 ? -d : d;                                                       \
    T w0, w1;                                                                 \
    if (d > 1 - 1e-6) { w0 = 1 - t; w1 = t; }                                 \
    else {                                                                    \
        T angle = ACOS(d), s = SIN(angle);                                    \
        w0 = SIN((1 - t) * angle) / s; w1 = SIN(t * angle) / s;              \
    }                                                                         \
    Q r = {w0 * a.vector + w1 * to.vector};                                   \
    return r;                                                                 \
}

SIMD_QUATERNION(simd_quatf, float, simd_float3, simd_float4, simd_float3x3, simd_float4x4,
                __builtin_sqrtf, __builtin_atan2f, __builtin_sinf, __builtin_cosf,
                __builtin_expf, __builtin_logf, __builtin_acosf)
SIMD_QUATERNION(simd_quatd, double, simd_double3, simd_double4, simd_double3x3, simd_double4x4,
                __builtin_sqrt, __builtin_atan2, __builtin_sin, __builtin_cos,
                __builtin_exp, __builtin_log, __builtin_acos)
#undef SIMD_QUATERNION

#endif
