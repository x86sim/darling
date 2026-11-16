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

#ifndef SIMD_MATRIX_H
#define SIMD_MATRIX_H

#include <simd/geometry.h>
#include <simd/matrix_types.h>
#include <simd/extern.h>
#include <simd/vector_make.h>

/*
 * Matrix functions. A simd_<type>CxR matrix has C columns of R elements:
 * simd_add, simd_sub, simd_mul (scalar, vector and matrix products),
 * simd_equal, simd_transpose, and for square matrices simd_determinant and
 * simd_inverse. simd_diagonal_matrix and simd_matrix build square matrices.
 */

/* Per (type, columns, rows) functions. */
#define SIMD_MATRIX(T, C, R)                                                  \
SIMD_FN simd_##T##C##x##R simd_add(simd_##T##C##x##R a, simd_##T##C##x##R b) { \
    simd_##T##C##x##R r;                                                      \
    for (int i = 0; i < C; i++) r.columns[i] = a.columns[i] + b.columns[i];   \
    return r;                                                                 \
}                                                                             \
SIMD_FN simd_##T##C##x##R simd_sub(simd_##T##C##x##R a, simd_##T##C##x##R b) { \
    simd_##T##C##x##R r;                                                      \
    for (int i = 0; i < C; i++) r.columns[i] = a.columns[i] - b.columns[i];   \
    return r;                                                                 \
}                                                                             \
SIMD_FN simd_##T##C##x##R simd_mul(T s, simd_##T##C##x##R m) {                 \
    simd_##T##C##x##R r;                                                      \
    for (int i = 0; i < C; i++) r.columns[i] = s * m.columns[i];              \
    return r;                                                                 \
}                                                                             \
SIMD_FN simd_##T##R simd_mul(simd_##T##C##x##R m, simd_##T##C v) {             \
    simd_##T##R r = (simd_##T##R)(T)0;                                        \
    for (int i = 0; i < C; i++) r += m.columns[i] * v[i];                     \
    return r;                                                                 \
}                                                                             \
SIMD_FN simd_##T##C simd_mul(simd_##T##R v, simd_##T##C##x##R m) {             \
    simd_##T##C r;                                                            \
    for (int i = 0; i < C; i++) r[i] = simd_dot(v, m.columns[i]);             \
    return r;                                                                 \
}                                                                             \
SIMD_FN simd_##T##R##x##C simd_transpose(simd_##T##C##x##R m) {                \
    simd_##T##R##x##C r;                                                      \
    for (int c = 0; c < C; c++)                                               \
        for (int rr = 0; rr < R; rr++) r.columns[rr][c] = m.columns[c][rr];   \
    return r;                                                                 \
}                                                                             \
SIMD_FN bool simd_equal(simd_##T##C##x##R a, simd_##T##C##x##R b) {           \
    for (int c = 0; c < C; c++)                                               \
        for (int rr = 0; rr < R; rr++)                                        \
            if (!(a.columns[c][rr] == b.columns[c][rr])) return 0;            \
    return 1;                                                                 \
}

/* The product of a C x R matrix with a K x C matrix is a K x R matrix. */
#define SIMD_MATRIX_PRODUCT(T, C, R, K)                                       \
SIMD_FN simd_##T##K##x##R simd_mul(simd_##T##C##x##R a, simd_##T##K##x##C b) { \
    simd_##T##K##x##R r;                                                      \
    for (int i = 0; i < K; i++) r.columns[i] = simd_mul(a, b.columns[i]);     \
    return r;                                                                 \
}

#define SIMD_MATRIX_ALL_ROWS(T, C)                                            \
    SIMD_MATRIX(T, C, 2) SIMD_MATRIX(T, C, 3) SIMD_MATRIX(T, C, 4)            \
    SIMD_MATRIX_PRODUCT(T, C, 2, 2) SIMD_MATRIX_PRODUCT(T, C, 2, 3) SIMD_MATRIX_PRODUCT(T, C, 2, 4) \
    SIMD_MATRIX_PRODUCT(T, C, 3, 2) SIMD_MATRIX_PRODUCT(T, C, 3, 3) SIMD_MATRIX_PRODUCT(T, C, 3, 4) \
    SIMD_MATRIX_PRODUCT(T, C, 4, 2) SIMD_MATRIX_PRODUCT(T, C, 4, 3) SIMD_MATRIX_PRODUCT(T, C, 4, 4)

#define SIMD_MATRIX_ALL(T) SIMD_MATRIX_ALL_ROWS(T, 2) SIMD_MATRIX_ALL_ROWS(T, 3) SIMD_MATRIX_ALL_ROWS(T, 4)

SIMD_MATRIX_ALL(float)
SIMD_MATRIX_ALL(double)

#undef SIMD_MATRIX_ALL
#undef SIMD_MATRIX_ALL_ROWS
#undef SIMD_MATRIX_PRODUCT
#undef SIMD_MATRIX

/*
 * Square matrices. The 2x2 and 3x3 determinants are the usual closed forms;
 * the 4x4 determinant expands along the first column. The inverse comes from
 * the system math library, which returns non-finite elements for a singular
 * matrix.
 */

#define SIMD_SQUARE(T, N, SUFFIX)                                             \
SIMD_FN simd_##T##N##x##N simd_inverse(simd_##T##N##x##N m) { return __invert_##SUFFIX(m); }

SIMD_SQUARE(float, 2, f2)
SIMD_SQUARE(float, 3, f3)
SIMD_SQUARE(float, 4, f4)
SIMD_SQUARE(double, 2, d2)
SIMD_SQUARE(double, 3, d3)
SIMD_SQUARE(double, 4, d4)
#undef SIMD_SQUARE

#define SIMD_DETERMINANT(T)                                                   \
SIMD_FN T simd_determinant(simd_##T##2x2 m) {                                  \
    return m.columns[0][0] * m.columns[1][1] - m.columns[1][0] * m.columns[0][1];\
}                                                                             \
SIMD_FN T simd_determinant(simd_##T##3x3 m) {                                  \
    return simd_dot(m.columns[0], simd_cross(m.columns[1], m.columns[2]));    \
}                                                                             \
SIMD_FN T simd_determinant(simd_##T##4x4 m) {                                  \
    T det = 0;                                                                \
    for (int row = 0; row < 4; row++) {                                       \
        int a = row == 0 ? 1 : 0, b = row <= 1 ? 2 : 1, c = row <= 2 ? 3 : 2; \
        simd_##T##3x3 minor;                                                  \
        for (int col = 1; col < 4; col++)                                     \
            minor.columns[col - 1] = (simd_##T##3){m.columns[col][a], m.columns[col][b], m.columns[col][c]};\
        T term = m.columns[0][row] * simd_determinant(minor);                 \
        det += (row & 1) ? -term : term;                                      \
    }                                                                         \
    return det;                                                               \
}                                                                             \
SIMD_FN simd_##T##2x2 simd_diagonal_matrix(simd_##T##2 d) {                    \
    simd_##T##2x2 r = {{(simd_##T##2){d.x, 0}, (simd_##T##2){0, d.y}}};        \
    return r;                                                                 \
}                                                                             \
SIMD_FN simd_##T##3x3 simd_diagonal_matrix(simd_##T##3 d) {                    \
    simd_##T##3x3 r = {{(simd_##T##3){d.x, 0, 0}, (simd_##T##3){0, d.y, 0}, (simd_##T##3){0, 0, d.z}}};\
    return r;                                                                 \
}                                                                             \
SIMD_FN simd_##T##4x4 simd_diagonal_matrix(simd_##T##4 d) {                    \
    simd_##T##4x4 r = {{(simd_##T##4){d.x, 0, 0, 0}, (simd_##T##4){0, d.y, 0, 0},\
                        (simd_##T##4){0, 0, d.z, 0}, (simd_##T##4){0, 0, 0, d.w}}};\
    return r;                                                                 \
}                                                                             \
SIMD_FN simd_##T##2x2 simd_matrix(simd_##T##2 c0, simd_##T##2 c1) {            \
    simd_##T##2x2 r = {{c0, c1}};                                              \
    return r;                                                                 \
}                                                                             \
SIMD_FN simd_##T##3x3 simd_matrix(simd_##T##3 c0, simd_##T##3 c1, simd_##T##3 c2) {\
    simd_##T##3x3 r = {{c0, c1, c2}};                                          \
    return r;                                                                 \
}                                                                             \
SIMD_FN simd_##T##4x4 simd_matrix(simd_##T##4 c0, simd_##T##4 c1, simd_##T##4 c2, simd_##T##4 c3) {\
    simd_##T##4x4 r = {{c0, c1, c2, c3}};                                      \
    return r;                                                                 \
}

SIMD_DETERMINANT(float)
SIMD_DETERMINANT(double)
#undef SIMD_DETERMINANT

#endif
