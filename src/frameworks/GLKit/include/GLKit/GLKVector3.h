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


#ifndef _GLKVector3_H_
#define _GLKVector3_H_

#include <GLKit/GLKMathTypes.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#ifdef __cplusplus
extern "C" {
#endif

GLK_INLINE GLKVector3 GLKVector3Make(float x, float y, float z)
{
    GLKVector3 v = { x, y, z };
    return v;
}

GLK_INLINE GLKVector3 GLKVector3MakeWithArray(float values[3])
{
    GLKVector3 v;
    for (int i = 0; i < 3; i++)
        v.v[i] = values[i];
    return v;
}

GLK_INLINE GLKVector3 GLKVector3Add(GLKVector3 a, GLKVector3 b)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = a.v[i] + b.v[i];
    return r;
}

GLK_INLINE GLKVector3 GLKVector3Subtract(GLKVector3 a, GLKVector3 b)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = a.v[i] - b.v[i];
    return r;
}

GLK_INLINE GLKVector3 GLKVector3Multiply(GLKVector3 a, GLKVector3 b)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = a.v[i] * b.v[i];
    return r;
}

GLK_INLINE GLKVector3 GLKVector3Divide(GLKVector3 a, GLKVector3 b)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = a.v[i] / b.v[i];
    return r;
}

GLK_INLINE GLKVector3 GLKVector3Negate(GLKVector3 a)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = -a.v[i];
    return r;
}

GLK_INLINE GLKVector3 GLKVector3AddScalar(GLKVector3 a, float value)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = a.v[i] + value;
    return r;
}

GLK_INLINE GLKVector3 GLKVector3SubtractScalar(GLKVector3 a, float value)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = a.v[i] - value;
    return r;
}

GLK_INLINE GLKVector3 GLKVector3MultiplyScalar(GLKVector3 a, float value)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = a.v[i] * value;
    return r;
}

GLK_INLINE GLKVector3 GLKVector3DivideScalar(GLKVector3 a, float value)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = a.v[i] / value;
    return r;
}

GLK_INLINE GLKVector3 GLKVector3Maximum(GLKVector3 a, GLKVector3 b)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = a.v[i] > b.v[i] ? a.v[i] : b.v[i];
    return r;
}

GLK_INLINE GLKVector3 GLKVector3Minimum(GLKVector3 a, GLKVector3 b)
{
    GLKVector3 r;
    for (int i = 0; i < 3; i++)
        r.v[i] = a.v[i] < b.v[i] ? a.v[i] : b.v[i];
    return r;
}

GLK_INLINE bool GLKVector3AllEqualToVector3(GLKVector3 a, GLKVector3 b)
{
    for (int i = 0; i < 3; i++)
        if (!(a.v[i] == b.v[i]))
            return false;
    return true;
}

GLK_INLINE bool GLKVector3AllEqualToScalar(GLKVector3 a, float value)
{
    for (int i = 0; i < 3; i++)
        if (!(a.v[i] == value))
            return false;
    return true;
}

GLK_INLINE bool GLKVector3AllGreaterThanVector3(GLKVector3 a, GLKVector3 b)
{
    for (int i = 0; i < 3; i++)
        if (!(a.v[i] > b.v[i]))
            return false;
    return true;
}

GLK_INLINE bool GLKVector3AllGreaterThanScalar(GLKVector3 a, float value)
{
    for (int i = 0; i < 3; i++)
        if (!(a.v[i] > value))
            return false;
    return true;
}

GLK_INLINE bool GLKVector3AllGreaterThanOrEqualToVector3(GLKVector3 a, GLKVector3 b)
{
    for (int i = 0; i < 3; i++)
        if (!(a.v[i] >= b.v[i]))
            return false;
    return true;
}

GLK_INLINE bool GLKVector3AllGreaterThanOrEqualToScalar(GLKVector3 a, float value)
{
    for (int i = 0; i < 3; i++)
        if (!(a.v[i] >= value))
            return false;
    return true;
}

GLK_INLINE float GLKVector3DotProduct(GLKVector3 a, GLKVector3 b)
{
    float d = 0.0f;
    for (int i = 0; i < 3; i++)
        d += a.v[i] * b.v[i];
    return d;
}

GLK_INLINE float GLKVector3Length(GLKVector3 a)
{
    return sqrtf(GLKVector3DotProduct(a, a));
}

GLK_INLINE GLKVector3 GLKVector3Normalize(GLKVector3 a)
{
    return GLKVector3MultiplyScalar(a, 1.0f / GLKVector3Length(a));
}

GLK_INLINE float GLKVector3Distance(GLKVector3 a, GLKVector3 b)
{
    return GLKVector3Length(GLKVector3Subtract(b, a));
}

GLK_INLINE GLKVector3 GLKVector3Lerp(GLKVector3 a, GLKVector3 b, float t)
{
    return GLKVector3Add(a, GLKVector3MultiplyScalar(GLKVector3Subtract(b, a), t));
}

GLK_INLINE GLKVector3 GLKVector3Project(GLKVector3 vectorToProject, GLKVector3 projectionVector)
{
    float scale = GLKVector3DotProduct(projectionVector, vectorToProject) / GLKVector3DotProduct(projectionVector, projectionVector);
    return GLKVector3MultiplyScalar(projectionVector, scale);
}

GLK_INLINE GLKVector3 GLKVector3CrossProduct(GLKVector3 a, GLKVector3 b)
{
    GLKVector3 r = { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    return r;
}

#ifdef __cplusplus
}
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
