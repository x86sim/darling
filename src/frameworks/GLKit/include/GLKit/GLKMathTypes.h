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

#ifndef _GLKMathTypes_H_
#define _GLKMathTypes_H_

#include <stdbool.h>
#include <stddef.h>
#include <math.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GLK_INLINE static __inline__

// Matrices are stored column-major; mCR is the element of column C and row R, so the
// member order is the memory order and `m` indexes the same storage as a flat array.

union _GLKMatrix2
{
    struct
    {
        float m00, m01;
        float m10, m11;
    };
    float m[4];
};
typedef union _GLKMatrix2 GLKMatrix2;

union _GLKMatrix3
{
    struct
    {
        float m00, m01, m02;
        float m10, m11, m12;
        float m20, m21, m22;
    };
    float m[9];
};
typedef union _GLKMatrix3 GLKMatrix3;

union _GLKMatrix4
{
    struct
    {
        float m00, m01, m02, m03;
        float m10, m11, m12, m13;
        float m20, m21, m22, m23;
        float m30, m31, m32, m33;
    };
    float m[16];
} __attribute__((aligned(16)));
typedef union _GLKMatrix4 GLKMatrix4;

union _GLKVector2
{
    struct { float x, y; };
    struct { float s, t; };
    float v[2];
} __attribute__((aligned(8)));
typedef union _GLKVector2 GLKVector2;

union _GLKVector3
{
    struct { float x, y, z; };
    struct { float r, g, b; };
    struct { float s, t, p; };
    float v[3];
};
typedef union _GLKVector3 GLKVector3;

union _GLKVector4
{
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    struct { float s, t, p, q; };
    float v[4];
} __attribute__((aligned(16)));
typedef union _GLKVector4 GLKVector4;

union _GLKQuaternion
{
    struct { GLKVector3 v; float s; };
    struct { float x, y, z, w; };
    float q[4];
} __attribute__((aligned(16)));
typedef union _GLKQuaternion GLKQuaternion;

extern const GLKMatrix3 GLKMatrix3Identity;
extern const GLKMatrix4 GLKMatrix4Identity;

#ifdef __cplusplus
}
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
