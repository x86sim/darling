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


#include <GLKit/GLKMath.h>

const GLKMatrix3 GLKMatrix3Identity = { { 1.0f, 0.0f, 0.0f,
                                          0.0f, 1.0f, 0.0f,
                                          0.0f, 0.0f, 1.0f } };
const GLKMatrix4 GLKMatrix4Identity = { { 1.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 1.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 1.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f, 1.0f } };

GLKMatrix3 GLKMatrix3Invert(GLKMatrix3 matrix, bool *isInvertible)
{
    // Cofactors of the transposed matrix, so that the result is the adjugate in column-major layout.
    const float *a = matrix.m;
    GLKMatrix3 inv;
    inv.m[0] = a[4] * a[8] - a[5] * a[7];
    inv.m[1] = a[2] * a[7] - a[1] * a[8];
    inv.m[2] = a[1] * a[5] - a[2] * a[4];
    inv.m[3] = a[5] * a[6] - a[3] * a[8];
    inv.m[4] = a[0] * a[8] - a[2] * a[6];
    inv.m[5] = a[2] * a[3] - a[0] * a[5];
    inv.m[6] = a[3] * a[7] - a[4] * a[6];
    inv.m[7] = a[1] * a[6] - a[0] * a[7];
    inv.m[8] = a[0] * a[4] - a[1] * a[3];
    float det = a[0] * inv.m[0] + a[3] * inv.m[1] + a[6] * inv.m[2];
    if (det == 0.0f || !isfinite(det))
    {
        if (isInvertible != NULL)
            *isInvertible = false;
        return GLKMatrix3Identity;
    }
    float scale = 1.0f / det;
    for (int i = 0; i < 9; i++)
        inv.m[i] *= scale;
    if (isInvertible != NULL)
        *isInvertible = true;
    return inv;
}

GLKMatrix3 GLKMatrix3InvertAndTranspose(GLKMatrix3 matrix, bool *isInvertible)
{
    return GLKMatrix3Transpose(GLKMatrix3Invert(matrix, isInvertible));
}

GLKMatrix4 GLKMatrix4Invert(GLKMatrix4 matrix, bool *isInvertible)
{
    // Gauss-Jordan elimination with partial pivoting on [matrix | identity], in double precision.
    double work[4][8];
    for (int c = 0; c < 4; c++)
    {
        for (int r = 0; r < 4; r++)
        {
            work[r][c] = matrix.m[c * 4 + r];
            work[r][4 + c] = (r == c) ? 1.0 : 0.0;
        }
    }
    for (int col = 0; col < 4; col++)
    {
        int pivot = col;
        for (int r = col + 1; r < 4; r++)
        {
            if (fabs(work[r][col]) > fabs(work[pivot][col]))
                pivot = r;
        }
        if (work[pivot][col] == 0.0 || !isfinite(work[pivot][col]))
        {
            if (isInvertible != NULL)
                *isInvertible = false;
            return GLKMatrix4Identity;
        }
        if (pivot != col)
        {
            for (int k = 0; k < 8; k++)
            {
                double tmp = work[col][k];
                work[col][k] = work[pivot][k];
                work[pivot][k] = tmp;
            }
        }
        double scale = 1.0 / work[col][col];
        for (int k = 0; k < 8; k++)
            work[col][k] *= scale;
        for (int r = 0; r < 4; r++)
        {
            if (r == col)
                continue;
            double factor = work[r][col];
            if (factor == 0.0)
                continue;
            for (int k = 0; k < 8; k++)
                work[r][k] -= factor * work[col][k];
        }
    }
    GLKMatrix4 result;
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            result.m[c * 4 + r] = (float)work[r][4 + c];
    if (isInvertible != NULL)
        *isInvertible = true;
    return result;
}

GLKMatrix4 GLKMatrix4InvertAndTranspose(GLKMatrix4 matrix, bool *isInvertible)
{
    return GLKMatrix4Transpose(GLKMatrix4Invert(matrix, isInvertible));
}

GLKVector3 GLKMathProject(GLKVector3 object, GLKMatrix4 model, GLKMatrix4 projection, int *viewport)
{
    GLKVector4 clip = GLKMatrix4MultiplyVector4(GLKMatrix4Multiply(projection, model),
                                                GLKVector4Make(object.x, object.y, object.z, 1.0f));
    float w = clip.w;
    GLKVector3 ndc = GLKVector3Make(clip.x / w, clip.y / w, clip.z / w);
    return GLKVector3Make(viewport[0] + viewport[2] * (ndc.x * 0.5f + 0.5f),
                          viewport[1] + viewport[3] * (ndc.y * 0.5f + 0.5f),
                          ndc.z * 0.5f + 0.5f);
}

// QUIRK: when the combined matrix is not invertible, *success is false and the returned vector is
// still computed, with the identity standing in for the inverse (GLKMatrix4Invert's singular result):
// it is not zero. Not documented. Evidence: black-box observation on macOS 26.7.3 x86_64 (tier 2).
// Test: test_quirk_unproject_singular_uses_identity in tests/src/glkit_math.c
GLKVector3 GLKMathUnproject(GLKVector3 window, GLKMatrix4 model, GLKMatrix4 projection, int *viewport, bool *success)
{
    bool invertible = false;
    GLKMatrix4 inverse = GLKMatrix4Invert(GLKMatrix4Multiply(projection, model), &invertible);
    if (success != NULL)
        *success = invertible;
    GLKVector4 ndc = GLKVector4Make((window.x - viewport[0]) / viewport[2] * 2.0f - 1.0f,
                                    (window.y - viewport[1]) / viewport[3] * 2.0f - 1.0f,
                                    window.z * 2.0f - 1.0f,
                                    1.0f);
    GLKVector4 object = GLKMatrix4MultiplyVector4(inverse, ndc);
    return GLKVector3Make(object.x / object.w, object.y / object.w, object.z / object.w);
}

// QUIRK: the result is not normalized, and a matrix with trace >= 0 (including the all-zero matrix,
// which gives (0, 0, 0, 0.5)) is always converted through the w component; scale 2 gives w = sqrt(7)/2.
// Not documented. Evidence: black-box observation on macOS 26.7.3 x86_64 (tier 2).
// Test: test_quirk_quaternion_from_matrix_not_normalized in tests/src/glkit_math.c
GLKQuaternion GLKQuaternionMakeWithMatrix3(GLKMatrix3 matrix)
{
    float m00 = matrix.m00, m11 = matrix.m11, m22 = matrix.m22;
    float trace = m00 + m11 + m22;
    GLKQuaternion q;
    if (trace >= 0.0f)
    {
        float s = 2.0f * sqrtf(trace + 1.0f);
        q.w = 0.25f * s;
        q.x = (matrix.m12 - matrix.m21) / s;
        q.y = (matrix.m20 - matrix.m02) / s;
        q.z = (matrix.m01 - matrix.m10) / s;
    }
    else if (m00 > m11 && m00 > m22)
    {
        float s = 2.0f * sqrtf(1.0f + m00 - m11 - m22);
        q.w = (matrix.m12 - matrix.m21) / s;
        q.x = 0.25f * s;
        q.y = (matrix.m10 + matrix.m01) / s;
        q.z = (matrix.m20 + matrix.m02) / s;
    }
    else if (m11 > m22)
    {
        float s = 2.0f * sqrtf(1.0f + m11 - m00 - m22);
        q.w = (matrix.m20 - matrix.m02) / s;
        q.x = (matrix.m10 + matrix.m01) / s;
        q.y = 0.25f * s;
        q.z = (matrix.m21 + matrix.m12) / s;
    }
    else
    {
        float s = 2.0f * sqrtf(1.0f + m22 - m00 - m11);
        q.w = (matrix.m01 - matrix.m10) / s;
        q.x = (matrix.m20 + matrix.m02) / s;
        q.y = (matrix.m21 + matrix.m12) / s;
        q.z = 0.25f * s;
    }
    return q;
}

GLKQuaternion GLKQuaternionMakeWithMatrix4(GLKMatrix4 matrix)
{
    return GLKQuaternionMakeWithMatrix3(GLKMatrix4GetMatrix3(matrix));
}

void GLKQuaternionRotateVector3Array(GLKQuaternion quaternion, GLKVector3 *vectors, size_t vectorCount)
{
    for (size_t i = 0; i < vectorCount; i++)
        vectors[i] = GLKQuaternionRotateVector3(quaternion, vectors[i]);
}

void GLKQuaternionRotateVector4Array(GLKQuaternion quaternion, GLKVector4 *vectors, size_t vectorCount)
{
    for (size_t i = 0; i < vectorCount; i++)
        vectors[i] = GLKQuaternionRotateVector4(quaternion, vectors[i]);
}

// QUIRK: a quaternion whose |w| exceeds 1 is normalized first, so the angle and axis stay defined;
// the axis of a rotation with |w| >= 1 (no rotation) is the zero vector rather than NaN, while the
// all-zero quaternion gives a NaN axis and the angle pi. Not documented. Evidence: black-box
// observation on macOS 26.7.3 x86_64 (tier 2), twelve inputs.
// Test: test_quirk_quaternion_angle_axis_edge_cases in tests/src/glkit_math.c
float GLKQuaternionAngle(GLKQuaternion quaternion)
{
    if (fabsf(quaternion.w) > 1.0f)
        quaternion = GLKQuaternionNormalize(quaternion);
    return 2.0f * acosf(quaternion.w);
}

GLKVector3 GLKQuaternionAxis(GLKQuaternion quaternion)
{
    if (fabsf(quaternion.w) > 1.0f)
        quaternion = GLKQuaternionNormalize(quaternion);
    if (fabsf(quaternion.w) >= 1.0f)
        return GLKVector3Make(0.0f, 0.0f, 0.0f);
    return GLKVector3Normalize(GLKVector3Make(quaternion.x, quaternion.y, quaternion.z));
}

// QUIRK: the blend is not re-normalized, so non-unit inputs give a non-unit result; the shorter arc
// is taken by negating the second operand when the dot product is negative, and t outside 0..1
// extrapolates. Not documented. Evidence: black-box observation on macOS 26.7.3 x86_64 (tier 2).
// Test: test_quirk_slerp_not_normalized in tests/src/glkit_math.c
GLKQuaternion GLKQuaternionSlerp(GLKQuaternion a, GLKQuaternion b, float t)
{
    float cosHalfTheta = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (cosHalfTheta < 0.0f)
    {
        b = GLKQuaternionMake(-b.x, -b.y, -b.z, -b.w);
        cosHalfTheta = -cosHalfTheta;
    }
    float ra, rb;
    if (cosHalfTheta >= 1.0f - 1e-6f)
    {
        // Nearly parallel: the sine of the angle vanishes, fall back to a linear blend.
        ra = 1.0f - t;
        rb = t;
    }
    else
    {
        float halfTheta = acosf(cosHalfTheta);
        float sinHalfTheta = sqrtf(1.0f - cosHalfTheta * cosHalfTheta);
        ra = sinf((1.0f - t) * halfTheta) / sinHalfTheta;
        rb = sinf(t * halfTheta) / sinHalfTheta;
    }
    return GLKQuaternionMake(a.x * ra + b.x * rb, a.y * ra + b.y * rb,
                             a.z * ra + b.z * rb, a.w * ra + b.w * rb);
}
