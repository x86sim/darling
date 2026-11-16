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


#ifndef _GLKQuaternion_H_
#define _GLKQuaternion_H_

#include <GLKit/GLKMathTypes.h>
#include <GLKit/GLKVector3.h>
#include <GLKit/GLKVector4.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static const GLKQuaternion GLKQuaternionIdentity = { 0.0f, 0.0f, 0.0f, 1.0f };

GLK_INLINE GLKQuaternion GLKQuaternionMake(float x, float y, float z, float w)
{
    GLKQuaternion q = { x, y, z, w };
    return q;
}

GLK_INLINE GLKQuaternion GLKQuaternionMakeWithVector3(GLKVector3 vector, float scalar)
{
    GLKQuaternion q = { vector.x, vector.y, vector.z, scalar };
    return q;
}

GLK_INLINE GLKQuaternion GLKQuaternionMakeWithArray(float values[4])
{
    GLKQuaternion q = { values[0], values[1], values[2], values[3] };
    return q;
}

// QUIRK: the axis is used as given, it is not normalized, so a non-unit axis yields a non-unit
// quaternion (axis (0,0,3), angle pi/2 gives z = 3 * sin(pi/4)). Not documented. Evidence: black-box
// observation on macOS 26.7.3 x86_64 (tier 2), both for this function and the Vector3Axis variant.
// Test: test_quirk_angle_axis_not_normalized in tests/src/glkit_math.c
GLK_INLINE GLKQuaternion GLKQuaternionMakeWithAngleAndAxis(float radians, float x, float y, float z)
{
    float half = radians * 0.5f;
    float scale = sinf(half);
    GLKQuaternion q = { scale * x, scale * y, scale * z, cosf(half) };
    return q;
}

GLK_INLINE GLKQuaternion GLKQuaternionMakeWithAngleAndVector3Axis(float radians, GLKVector3 axisVector)
{
    return GLKQuaternionMakeWithAngleAndAxis(radians, axisVector.x, axisVector.y, axisVector.z);
}

GLK_INLINE GLKQuaternion GLKQuaternionAdd(GLKQuaternion a, GLKQuaternion b)
{
    GLKQuaternion q = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    return q;
}

GLK_INLINE GLKQuaternion GLKQuaternionSubtract(GLKQuaternion a, GLKQuaternion b)
{
    GLKQuaternion q = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    return q;
}

GLK_INLINE GLKQuaternion GLKQuaternionMultiply(GLKQuaternion l, GLKQuaternion r)
{
    GLKQuaternion q = { l.w * r.x + l.x * r.w + l.y * r.z - l.z * r.y,
                        l.w * r.y + l.y * r.w + l.z * r.x - l.x * r.z,
                        l.w * r.z + l.z * r.w + l.x * r.y - l.y * r.x,
                        l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z };
    return q;
}

GLK_INLINE float GLKQuaternionLength(GLKQuaternion q)
{
    return sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

GLK_INLINE GLKQuaternion GLKQuaternionConjugate(GLKQuaternion q)
{
    GLKQuaternion c = { -q.x, -q.y, -q.z, q.w };
    return c;
}

GLK_INLINE GLKQuaternion GLKQuaternionNormalize(GLKQuaternion q)
{
    float scale = 1.0f / GLKQuaternionLength(q);
    GLKQuaternion n = { q.x * scale, q.y * scale, q.z * scale, q.w * scale };
    return n;
}

GLK_INLINE GLKQuaternion GLKQuaternionInvert(GLKQuaternion q)
{
    float scale = 1.0f / (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    GLKQuaternion i = { -q.x * scale, -q.y * scale, -q.z * scale, q.w * scale };
    return i;
}

GLK_INLINE GLKVector3 GLKQuaternionRotateVector3(GLKQuaternion quaternion, GLKVector3 vector)
{
    GLKQuaternion r = GLKQuaternionMultiply(GLKQuaternionMultiply(quaternion, GLKQuaternionMake(vector.x, vector.y, vector.z, 0.0f)),
                                            GLKQuaternionInvert(quaternion));
    return GLKVector3Make(r.x, r.y, r.z);
}

GLK_INLINE GLKVector4 GLKQuaternionRotateVector4(GLKQuaternion quaternion, GLKVector4 vector)
{
    GLKQuaternion r = GLKQuaternionMultiply(GLKQuaternionMultiply(quaternion, GLKQuaternionMake(vector.x, vector.y, vector.z, 0.0f)),
                                            GLKQuaternionInvert(quaternion));
    return GLKVector4Make(r.x, r.y, r.z, vector.w);
}

// These are exported by the framework rather than inlined.
extern GLKQuaternion GLKQuaternionMakeWithMatrix3(GLKMatrix3 matrix);
extern GLKQuaternion GLKQuaternionMakeWithMatrix4(GLKMatrix4 matrix);
extern void GLKQuaternionRotateVector3Array(GLKQuaternion quaternion, GLKVector3 *vectors, size_t vectorCount);
extern void GLKQuaternionRotateVector4Array(GLKQuaternion quaternion, GLKVector4 *vectors, size_t vectorCount);
extern float GLKQuaternionAngle(GLKQuaternion quaternion);
extern GLKVector3 GLKQuaternionAxis(GLKQuaternion quaternion);
// Spherical linear interpolation along the shorter arc. The result is not re-normalized
// (see the QUIRK in GLKMatrix.m).
extern GLKQuaternion GLKQuaternionSlerp(GLKQuaternion a, GLKQuaternion b, float t);

#ifdef __cplusplus
}
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
