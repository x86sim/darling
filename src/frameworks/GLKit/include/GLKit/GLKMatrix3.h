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


#ifndef _GLKMatrix3_H_
#define _GLKMatrix3_H_

#include <GLKit/GLKMathTypes.h>
#include <GLKit/GLKVector2.h>
#include <GLKit/GLKVector3.h>
#include <GLKit/GLKVector4.h>
#include <GLKit/GLKQuaternion.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#ifdef __cplusplus
extern "C" {
#endif

GLK_INLINE GLKMatrix3 GLKMatrix3Make(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
{
    GLKMatrix3 m = { m00, m01, m02, m10, m11, m12, m20, m21, m22 };
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeAndTranspose(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
{
    GLKMatrix3 m = { m00, m10, m20, m01, m11, m21, m02, m12, m22 };
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeWithArray(float values[9])
{
    GLKMatrix3 m;
    for (int i = 0; i < 9; i++)
        m.m[i] = values[i];
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeWithArrayAndTranspose(float values[9])
{
    GLKMatrix3 m;
    for (int c = 0; c < 3; c++)
        for (int r = 0; r < 3; r++)
            m.m[c * 3 + r] = values[r * 3 + c];
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeWithRows(GLKVector3 row0, GLKVector3 row1, GLKVector3 row2)
{
    GLKMatrix3 m;
    for (int c = 0; c < 3; c++)
    {
        m.m[c * 3 + 0] = row0.v[c];
        m.m[c * 3 + 1] = row1.v[c];
        m.m[c * 3 + 2] = row2.v[c];
    }
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeWithColumns(GLKVector3 col0, GLKVector3 col1, GLKVector3 col2)
{
    GLKMatrix3 m;
    for (int r = 0; r < 3; r++)
    {
        m.m[0 * 3 + r] = col0.v[r];
        m.m[1 * 3 + r] = col1.v[r];
        m.m[2 * 3 + r] = col2.v[r];
    }
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeScale(float sx, float sy, float sz)
{
    GLKMatrix3 m = GLKMatrix3Identity;
    m.m00 = sx;
    m.m11 = sy;
    m.m22 = sz;
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeRotation(float radians, float x, float y, float z)
{
    GLKVector3 v = GLKVector3Normalize(GLKVector3Make(x, y, z));
    float c = cosf(radians);
    float s = sinf(radians);
    float k = 1.0f - c;
    GLKMatrix3 m = GLKMatrix3Identity;
    m.m00 = c + k * v.x * v.x;
    m.m01 = k * v.x * v.y + s * v.z;
    m.m02 = k * v.x * v.z - s * v.y;
    m.m10 = k * v.x * v.y - s * v.z;
    m.m11 = c + k * v.y * v.y;
    m.m12 = k * v.y * v.z + s * v.x;
    m.m20 = k * v.x * v.z + s * v.y;
    m.m21 = k * v.y * v.z - s * v.x;
    m.m22 = c + k * v.z * v.z;
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeXRotation(float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);
    GLKMatrix3 m = GLKMatrix3Identity;
    m.m11 = c;
    m.m12 = s;
    m.m21 = -s;
    m.m22 = c;
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeYRotation(float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);
    GLKMatrix3 m = GLKMatrix3Identity;
    m.m22 = c;
    m.m20 = s;
    m.m02 = -s;
    m.m00 = c;
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeZRotation(float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);
    GLKMatrix3 m = GLKMatrix3Identity;
    m.m00 = c;
    m.m01 = s;
    m.m10 = -s;
    m.m11 = c;
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3MakeWithQuaternion(GLKQuaternion quaternion)
{
    GLKQuaternion q = GLKQuaternionNormalize(quaternion);
    float x2 = q.x + q.x, y2 = q.y + q.y, z2 = q.z + q.z;
    float xx = q.x * x2, xy = q.x * y2, xz = q.x * z2;
    float yy = q.y * y2, yz = q.y * z2, zz = q.z * z2;
    float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;
    GLKMatrix3 m = GLKMatrix3Identity;
    m.m00 = 1.0f - (yy + zz);
    m.m01 = xy + wz;
    m.m02 = xz - wy;
    m.m10 = xy - wz;
    m.m11 = 1.0f - (xx + zz);
    m.m12 = yz + wx;
    m.m20 = xz + wy;
    m.m21 = yz - wx;
    m.m22 = 1.0f - (xx + yy);
    return m;
}

GLK_INLINE GLKMatrix2 GLKMatrix3GetMatrix2(GLKMatrix3 matrix)
{
    GLKMatrix2 m = { matrix.m00, matrix.m01,
                     matrix.m10, matrix.m11 };
    return m;
}

GLK_INLINE GLKVector3 GLKMatrix3GetRow(GLKMatrix3 matrix, int row)
{
    GLKVector3 v;
    for (int c = 0; c < 3; c++)
        v.v[c] = matrix.m[c * 3 + row];
    return v;
}

GLK_INLINE GLKVector3 GLKMatrix3GetColumn(GLKMatrix3 matrix, int column)
{
    GLKVector3 v;
    for (int r = 0; r < 3; r++)
        v.v[r] = matrix.m[column * 3 + r];
    return v;
}

GLK_INLINE GLKMatrix3 GLKMatrix3SetRow(GLKMatrix3 matrix, int row, GLKVector3 vector)
{
    for (int c = 0; c < 3; c++)
        matrix.m[c * 3 + row] = vector.v[c];
    return matrix;
}

GLK_INLINE GLKMatrix3 GLKMatrix3SetColumn(GLKMatrix3 matrix, int column, GLKVector3 vector)
{
    for (int r = 0; r < 3; r++)
        matrix.m[column * 3 + r] = vector.v[r];
    return matrix;
}

GLK_INLINE GLKMatrix3 GLKMatrix3Transpose(GLKMatrix3 matrix)
{
    GLKMatrix3 t;
    for (int c = 0; c < 3; c++)
        for (int r = 0; r < 3; r++)
            t.m[c * 3 + r] = matrix.m[r * 3 + c];
    return t;
}

// Not inline: inverse of the matrix; when it is singular the identity is returned and *isInvertible is set to false.
extern GLKMatrix3 GLKMatrix3Invert(GLKMatrix3 matrix, bool *isInvertible);
extern GLKMatrix3 GLKMatrix3InvertAndTranspose(GLKMatrix3 matrix, bool *isInvertible);

GLK_INLINE GLKMatrix3 GLKMatrix3Multiply(GLKMatrix3 l, GLKMatrix3 r)
{
    GLKMatrix3 m;
    for (int c = 0; c < 3; c++)
        for (int row = 0; row < 3; row++)
        {
            float s = 0.0f;
            for (int k = 0; k < 3; k++)
                s += l.m[k * 3 + row] * r.m[c * 3 + k];
            m.m[c * 3 + row] = s;
        }
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3Add(GLKMatrix3 l, GLKMatrix3 r)
{
    GLKMatrix3 m;
    for (int i = 0; i < 9; i++)
        m.m[i] = l.m[i] + r.m[i];
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3Subtract(GLKMatrix3 l, GLKMatrix3 r)
{
    GLKMatrix3 m;
    for (int i = 0; i < 9; i++)
        m.m[i] = l.m[i] - r.m[i];
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix3Scale(GLKMatrix3 matrix, float sx, float sy, float sz)
{
    return GLKMatrix3Multiply(matrix, GLKMatrix3MakeScale(sx, sy, sz));
}

GLK_INLINE GLKMatrix3 GLKMatrix3ScaleWithVector3(GLKMatrix3 matrix, GLKVector3 scaleVector)
{
    return GLKMatrix3Scale(matrix, scaleVector.x, scaleVector.y, scaleVector.z);
}

GLK_INLINE GLKMatrix3 GLKMatrix3Rotate(GLKMatrix3 matrix, float radians, float x, float y, float z)
{
    return GLKMatrix3Multiply(matrix, GLKMatrix3MakeRotation(radians, x, y, z));
}

GLK_INLINE GLKMatrix3 GLKMatrix3RotateWithVector3(GLKMatrix3 matrix, float radians, GLKVector3 axisVector)
{
    return GLKMatrix3Multiply(matrix, GLKMatrix3MakeRotation(radians, axisVector.x, axisVector.y, axisVector.z));
}

GLK_INLINE GLKMatrix3 GLKMatrix3RotateX(GLKMatrix3 matrix, float radians)
{
    return GLKMatrix3Multiply(matrix, GLKMatrix3MakeXRotation(radians));
}

GLK_INLINE GLKMatrix3 GLKMatrix3RotateY(GLKMatrix3 matrix, float radians)
{
    return GLKMatrix3Multiply(matrix, GLKMatrix3MakeYRotation(radians));
}

GLK_INLINE GLKMatrix3 GLKMatrix3RotateZ(GLKMatrix3 matrix, float radians)
{
    return GLKMatrix3Multiply(matrix, GLKMatrix3MakeZRotation(radians));
}

GLK_INLINE GLKVector3 GLKMatrix3MultiplyVector3(GLKMatrix3 matrixLeft, GLKVector3 vectorRight)
{
    GLKVector3 v = { matrixLeft.m00 * vectorRight.x + matrixLeft.m10 * vectorRight.y + matrixLeft.m20 * vectorRight.z,
                     matrixLeft.m01 * vectorRight.x + matrixLeft.m11 * vectorRight.y + matrixLeft.m21 * vectorRight.z,
                     matrixLeft.m02 * vectorRight.x + matrixLeft.m12 * vectorRight.y + matrixLeft.m22 * vectorRight.z };
    return v;
}

GLK_INLINE void GLKMatrix3MultiplyVector3Array(GLKMatrix3 matrix, GLKVector3 *vectors, size_t vectorCount)
{
    for (size_t i = 0; i < vectorCount; i++)
        vectors[i] = GLKMatrix3MultiplyVector3(matrix, vectors[i]);
}

#ifdef __cplusplus
}
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
