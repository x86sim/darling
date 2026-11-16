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


#ifndef _GLKMatrix4_H_
#define _GLKMatrix4_H_

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

GLK_INLINE GLKMatrix4 GLKMatrix4Make(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33)
{
    GLKMatrix4 m = { m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33 };
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeAndTranspose(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33)
{
    GLKMatrix4 m = { m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03, m13, m23, m33 };
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeWithArray(float values[16])
{
    GLKMatrix4 m;
    for (int i = 0; i < 16; i++)
        m.m[i] = values[i];
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeWithArrayAndTranspose(float values[16])
{
    GLKMatrix4 m;
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            m.m[c * 4 + r] = values[r * 4 + c];
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeWithRows(GLKVector4 row0, GLKVector4 row1, GLKVector4 row2, GLKVector4 row3)
{
    GLKMatrix4 m;
    for (int c = 0; c < 4; c++)
    {
        m.m[c * 4 + 0] = row0.v[c];
        m.m[c * 4 + 1] = row1.v[c];
        m.m[c * 4 + 2] = row2.v[c];
        m.m[c * 4 + 3] = row3.v[c];
    }
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeWithColumns(GLKVector4 col0, GLKVector4 col1, GLKVector4 col2, GLKVector4 col3)
{
    GLKMatrix4 m;
    for (int r = 0; r < 4; r++)
    {
        m.m[0 * 4 + r] = col0.v[r];
        m.m[1 * 4 + r] = col1.v[r];
        m.m[2 * 4 + r] = col2.v[r];
        m.m[3 * 4 + r] = col3.v[r];
    }
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeTranslation(float tx, float ty, float tz)
{
    GLKMatrix4 m = GLKMatrix4Identity;
    m.m30 = tx;
    m.m31 = ty;
    m.m32 = tz;
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeScale(float sx, float sy, float sz)
{
    GLKMatrix4 m = GLKMatrix4Identity;
    m.m00 = sx;
    m.m11 = sy;
    m.m22 = sz;
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeRotation(float radians, float x, float y, float z)
{
    GLKVector3 v = GLKVector3Normalize(GLKVector3Make(x, y, z));
    float c = cosf(radians);
    float s = sinf(radians);
    float k = 1.0f - c;
    GLKMatrix4 m = GLKMatrix4Identity;
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

GLK_INLINE GLKMatrix4 GLKMatrix4MakeXRotation(float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);
    GLKMatrix4 m = GLKMatrix4Identity;
    m.m11 = c;
    m.m12 = s;
    m.m21 = -s;
    m.m22 = c;
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeYRotation(float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);
    GLKMatrix4 m = GLKMatrix4Identity;
    m.m22 = c;
    m.m20 = s;
    m.m02 = -s;
    m.m00 = c;
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeZRotation(float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);
    GLKMatrix4 m = GLKMatrix4Identity;
    m.m00 = c;
    m.m01 = s;
    m.m10 = -s;
    m.m11 = c;
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeWithQuaternion(GLKQuaternion quaternion)
{
    GLKQuaternion q = GLKQuaternionNormalize(quaternion);
    float x2 = q.x + q.x, y2 = q.y + q.y, z2 = q.z + q.z;
    float xx = q.x * x2, xy = q.x * y2, xz = q.x * z2;
    float yy = q.y * y2, yz = q.y * z2, zz = q.z * z2;
    float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;
    GLKMatrix4 m = GLKMatrix4Identity;
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

GLK_INLINE GLKMatrix4 GLKMatrix4MakePerspective(float fovyRadians, float aspect, float nearZ, float farZ)
{
    float cotan = 1.0f / tanf(fovyRadians / 2.0f);
    GLKMatrix4 m = { cotan / aspect, 0.0f, 0.0f, 0.0f,
                     0.0f, cotan, 0.0f, 0.0f,
                     0.0f, 0.0f, (farZ + nearZ) / (nearZ - farZ), -1.0f,
                     0.0f, 0.0f, (2.0f * farZ * nearZ) / (nearZ - farZ), 0.0f };
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeFrustum(float left, float right, float bottom, float top, float nearZ, float farZ)
{
    float ral = right + left;
    float rsl = right - left;
    float tsb = top - bottom;
    float tab = top + bottom;
    float fan = farZ + nearZ;
    float fsn = farZ - nearZ;
    GLKMatrix4 m = { 2.0f * nearZ / rsl, 0.0f, 0.0f, 0.0f,
                     0.0f, 2.0f * nearZ / tsb, 0.0f, 0.0f,
                     ral / rsl, tab / tsb, -fan / fsn, -1.0f,
                     0.0f, 0.0f, (-2.0f * farZ * nearZ) / fsn, 0.0f };
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeOrtho(float left, float right, float bottom, float top, float nearZ, float farZ)
{
    float ral = right + left;
    float rsl = right - left;
    float tab = top + bottom;
    float tsb = top - bottom;
    float fan = farZ + nearZ;
    float fsn = farZ - nearZ;
    GLKMatrix4 m = { 2.0f / rsl, 0.0f, 0.0f, 0.0f,
                     0.0f, 2.0f / tsb, 0.0f, 0.0f,
                     0.0f, 0.0f, -2.0f / fsn, 0.0f,
                     -ral / rsl, -tab / tsb, -fan / fsn, 1.0f };
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4MakeLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
{
    GLKVector3 ev = { eyeX, eyeY, eyeZ };
    GLKVector3 cv = { centerX, centerY, centerZ };
    GLKVector3 uv = { upX, upY, upZ };
    GLKVector3 n = GLKVector3Normalize(GLKVector3Add(ev, GLKVector3Negate(cv)));
    GLKVector3 u = GLKVector3Normalize(GLKVector3CrossProduct(uv, n));
    GLKVector3 v = GLKVector3CrossProduct(n, u);
    GLKMatrix4 m = { u.v[0], v.v[0], n.v[0], 0.0f,
                     u.v[1], v.v[1], n.v[1], 0.0f,
                     u.v[2], v.v[2], n.v[2], 0.0f,
                     GLKVector3DotProduct(GLKVector3Negate(u), ev),
                     GLKVector3DotProduct(GLKVector3Negate(v), ev),
                     GLKVector3DotProduct(GLKVector3Negate(n), ev),
                     1.0f };
    return m;
}

GLK_INLINE GLKMatrix3 GLKMatrix4GetMatrix3(GLKMatrix4 matrix)
{
    GLKMatrix3 m = { matrix.m00, matrix.m01, matrix.m02,
                     matrix.m10, matrix.m11, matrix.m12,
                     matrix.m20, matrix.m21, matrix.m22 };
    return m;
}

GLK_INLINE GLKMatrix2 GLKMatrix4GetMatrix2(GLKMatrix4 matrix)
{
    GLKMatrix2 m = { matrix.m00, matrix.m01,
                     matrix.m10, matrix.m11 };
    return m;
}

GLK_INLINE GLKVector4 GLKMatrix4GetRow(GLKMatrix4 matrix, int row)
{
    GLKVector4 v;
    for (int c = 0; c < 4; c++)
        v.v[c] = matrix.m[c * 4 + row];
    return v;
}

GLK_INLINE GLKVector4 GLKMatrix4GetColumn(GLKMatrix4 matrix, int column)
{
    GLKVector4 v;
    for (int r = 0; r < 4; r++)
        v.v[r] = matrix.m[column * 4 + r];
    return v;
}

GLK_INLINE GLKMatrix4 GLKMatrix4SetRow(GLKMatrix4 matrix, int row, GLKVector4 vector)
{
    for (int c = 0; c < 4; c++)
        matrix.m[c * 4 + row] = vector.v[c];
    return matrix;
}

GLK_INLINE GLKMatrix4 GLKMatrix4SetColumn(GLKMatrix4 matrix, int column, GLKVector4 vector)
{
    for (int r = 0; r < 4; r++)
        matrix.m[column * 4 + r] = vector.v[r];
    return matrix;
}

GLK_INLINE GLKMatrix4 GLKMatrix4Transpose(GLKMatrix4 matrix)
{
    GLKMatrix4 t;
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            t.m[c * 4 + r] = matrix.m[r * 4 + c];
    return t;
}

// Not inline: inverse of the matrix; when it is singular the identity is returned and *isInvertible is set to false.
extern GLKMatrix4 GLKMatrix4Invert(GLKMatrix4 matrix, bool *isInvertible);
extern GLKMatrix4 GLKMatrix4InvertAndTranspose(GLKMatrix4 matrix, bool *isInvertible);

GLK_INLINE GLKMatrix4 GLKMatrix4Multiply(GLKMatrix4 l, GLKMatrix4 r)
{
    GLKMatrix4 m;
    for (int c = 0; c < 4; c++)
        for (int row = 0; row < 4; row++)
        {
            float s = 0.0f;
            for (int k = 0; k < 4; k++)
                s += l.m[k * 4 + row] * r.m[c * 4 + k];
            m.m[c * 4 + row] = s;
        }
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4Add(GLKMatrix4 l, GLKMatrix4 r)
{
    GLKMatrix4 m;
    for (int i = 0; i < 16; i++)
        m.m[i] = l.m[i] + r.m[i];
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4Subtract(GLKMatrix4 l, GLKMatrix4 r)
{
    GLKMatrix4 m;
    for (int i = 0; i < 16; i++)
        m.m[i] = l.m[i] - r.m[i];
    return m;
}

GLK_INLINE GLKMatrix4 GLKMatrix4Rotate(GLKMatrix4 matrix, float radians, float x, float y, float z)
{
    return GLKMatrix4Multiply(matrix, GLKMatrix4MakeRotation(radians, x, y, z));
}

GLK_INLINE GLKMatrix4 GLKMatrix4RotateWithVector3(GLKMatrix4 matrix, float radians, GLKVector3 axisVector)
{
    return GLKMatrix4Multiply(matrix, GLKMatrix4MakeRotation(radians, axisVector.x, axisVector.y, axisVector.z));
}

GLK_INLINE GLKMatrix4 GLKMatrix4RotateX(GLKMatrix4 matrix, float radians)
{
    return GLKMatrix4Multiply(matrix, GLKMatrix4MakeXRotation(radians));
}

GLK_INLINE GLKMatrix4 GLKMatrix4RotateY(GLKMatrix4 matrix, float radians)
{
    return GLKMatrix4Multiply(matrix, GLKMatrix4MakeYRotation(radians));
}

GLK_INLINE GLKMatrix4 GLKMatrix4RotateZ(GLKMatrix4 matrix, float radians)
{
    return GLKMatrix4Multiply(matrix, GLKMatrix4MakeZRotation(radians));
}

GLK_INLINE GLKMatrix4 GLKMatrix4RotateWithVector4(GLKMatrix4 matrix, float radians, GLKVector4 axisVector)
{
    return GLKMatrix4Multiply(matrix, GLKMatrix4MakeRotation(radians, axisVector.x, axisVector.y, axisVector.z));
}

GLK_INLINE GLKMatrix4 GLKMatrix4Translate(GLKMatrix4 matrix, float tx, float ty, float tz)
{
    return GLKMatrix4Multiply(matrix, GLKMatrix4MakeTranslation(tx, ty, tz));
}

GLK_INLINE GLKMatrix4 GLKMatrix4TranslateWithVector3(GLKMatrix4 matrix, GLKVector3 translationVector)
{
    return GLKMatrix4Translate(matrix, translationVector.x, translationVector.y, translationVector.z);
}

GLK_INLINE GLKMatrix4 GLKMatrix4TranslateWithVector4(GLKMatrix4 matrix, GLKVector4 translationVector)
{
    return GLKMatrix4Translate(matrix, translationVector.x, translationVector.y, translationVector.z);
}

GLK_INLINE GLKMatrix4 GLKMatrix4Scale(GLKMatrix4 matrix, float sx, float sy, float sz)
{
    return GLKMatrix4Multiply(matrix, GLKMatrix4MakeScale(sx, sy, sz));
}

GLK_INLINE GLKMatrix4 GLKMatrix4ScaleWithVector3(GLKMatrix4 matrix, GLKVector3 scaleVector)
{
    return GLKMatrix4Scale(matrix, scaleVector.x, scaleVector.y, scaleVector.z);
}

GLK_INLINE GLKMatrix4 GLKMatrix4ScaleWithVector4(GLKMatrix4 matrix, GLKVector4 scaleVector)
{
    return GLKMatrix4Scale(matrix, scaleVector.x, scaleVector.y, scaleVector.z);
}

GLK_INLINE GLKVector4 GLKMatrix4MultiplyVector4(GLKMatrix4 matrixLeft, GLKVector4 vectorRight)
{
    GLKVector4 v = { matrixLeft.m00 * vectorRight.x + matrixLeft.m10 * vectorRight.y + matrixLeft.m20 * vectorRight.z + matrixLeft.m30 * vectorRight.w,
                     matrixLeft.m01 * vectorRight.x + matrixLeft.m11 * vectorRight.y + matrixLeft.m21 * vectorRight.z + matrixLeft.m31 * vectorRight.w,
                     matrixLeft.m02 * vectorRight.x + matrixLeft.m12 * vectorRight.y + matrixLeft.m22 * vectorRight.z + matrixLeft.m32 * vectorRight.w,
                     matrixLeft.m03 * vectorRight.x + matrixLeft.m13 * vectorRight.y + matrixLeft.m23 * vectorRight.z + matrixLeft.m33 * vectorRight.w };
    return v;
}

GLK_INLINE GLKVector3 GLKMatrix4MultiplyVector3(GLKMatrix4 matrixLeft, GLKVector3 vectorRight)
{
    GLKVector4 v4 = GLKMatrix4MultiplyVector4(matrixLeft, GLKVector4Make(vectorRight.x, vectorRight.y, vectorRight.z, 0.0f));
    return GLKVector3Make(v4.x, v4.y, v4.z);
}

GLK_INLINE GLKVector3 GLKMatrix4MultiplyVector3WithTranslation(GLKMatrix4 matrixLeft, GLKVector3 vectorRight)
{
    GLKVector4 v4 = GLKMatrix4MultiplyVector4(matrixLeft, GLKVector4Make(vectorRight.x, vectorRight.y, vectorRight.z, 1.0f));
    return GLKVector3Make(v4.x, v4.y, v4.z);
}

GLK_INLINE GLKVector3 GLKMatrix4MultiplyAndProjectVector3(GLKMatrix4 matrixLeft, GLKVector3 vectorRight)
{
    GLKVector4 v4 = GLKMatrix4MultiplyVector4(matrixLeft, GLKVector4Make(vectorRight.x, vectorRight.y, vectorRight.z, 1.0f));
    return GLKVector3MultiplyScalar(GLKVector3Make(v4.x, v4.y, v4.z), 1.0f / v4.w);
}

GLK_INLINE void GLKMatrix4MultiplyVector3Array(GLKMatrix4 matrix, GLKVector3 *vectors, size_t vectorCount)
{
    for (size_t i = 0; i < vectorCount; i++)
        vectors[i] = GLKMatrix4MultiplyVector3(matrix, vectors[i]);
}

GLK_INLINE void GLKMatrix4MultiplyVector3ArrayWithTranslation(GLKMatrix4 matrix, GLKVector3 *vectors, size_t vectorCount)
{
    for (size_t i = 0; i < vectorCount; i++)
        vectors[i] = GLKMatrix4MultiplyVector3WithTranslation(matrix, vectors[i]);
}

GLK_INLINE void GLKMatrix4MultiplyAndProjectVector3Array(GLKMatrix4 matrix, GLKVector3 *vectors, size_t vectorCount)
{
    for (size_t i = 0; i < vectorCount; i++)
        vectors[i] = GLKMatrix4MultiplyAndProjectVector3(matrix, vectors[i]);
}

GLK_INLINE void GLKMatrix4MultiplyVector4Array(GLKMatrix4 matrix, GLKVector4 *vectors, size_t vectorCount)
{
    for (size_t i = 0; i < vectorCount; i++)
        vectors[i] = GLKMatrix4MultiplyVector4(matrix, vectors[i]);
}

#ifdef __cplusplus
}
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
