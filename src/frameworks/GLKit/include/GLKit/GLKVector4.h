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


#ifndef _GLKVector4_H_
#define _GLKVector4_H_

#include <GLKit/GLKMathTypes.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#ifdef __cplusplus
extern "C" {
#endif

GLK_INLINE GLKVector4 GLKVector4Make(float x, float y, float z, float w)
{
    GLKVector4 v = { x, y, z, w };
    return v;
}

GLK_INLINE GLKVector4 GLKVector4MakeWithArray(float values[4])
{
    GLKVector4 v;
    for (int i = 0; i < 4; i++)
        v.v[i] = values[i];
    return v;
}

GLK_INLINE GLKVector4 GLKVector4Add(GLKVector4 a, GLKVector4 b)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] + b.v[i];
    return r;
}

GLK_INLINE GLKVector4 GLKVector4Subtract(GLKVector4 a, GLKVector4 b)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] - b.v[i];
    return r;
}

GLK_INLINE GLKVector4 GLKVector4Multiply(GLKVector4 a, GLKVector4 b)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] * b.v[i];
    return r;
}

GLK_INLINE GLKVector4 GLKVector4Divide(GLKVector4 a, GLKVector4 b)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] / b.v[i];
    return r;
}

GLK_INLINE GLKVector4 GLKVector4Negate(GLKVector4 a)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = -a.v[i];
    return r;
}

GLK_INLINE GLKVector4 GLKVector4AddScalar(GLKVector4 a, float value)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] + value;
    return r;
}

GLK_INLINE GLKVector4 GLKVector4SubtractScalar(GLKVector4 a, float value)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] - value;
    return r;
}

GLK_INLINE GLKVector4 GLKVector4MultiplyScalar(GLKVector4 a, float value)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] * value;
    return r;
}

GLK_INLINE GLKVector4 GLKVector4DivideScalar(GLKVector4 a, float value)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] / value;
    return r;
}

GLK_INLINE GLKVector4 GLKVector4Maximum(GLKVector4 a, GLKVector4 b)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] > b.v[i] ? a.v[i] : b.v[i];
    return r;
}

GLK_INLINE GLKVector4 GLKVector4Minimum(GLKVector4 a, GLKVector4 b)
{
    GLKVector4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] < b.v[i] ? a.v[i] : b.v[i];
    return r;
}

GLK_INLINE bool GLKVector4AllEqualToVector4(GLKVector4 a, GLKVector4 b)
{
    for (int i = 0; i < 4; i++)
        if (!(a.v[i] == b.v[i]))
            return false;
    return true;
}

GLK_INLINE bool GLKVector4AllEqualToScalar(GLKVector4 a, float value)
{
    for (int i = 0; i < 4; i++)
        if (!(a.v[i] == value))
            return false;
    return true;
}

GLK_INLINE bool GLKVector4AllGreaterThanVector4(GLKVector4 a, GLKVector4 b)
{
    for (int i = 0; i < 4; i++)
        if (!(a.v[i] > b.v[i]))
            return false;
    return true;
}

GLK_INLINE bool GLKVector4AllGreaterThanScalar(GLKVector4 a, float value)
{
    for (int i = 0; i < 4; i++)
        if (!(a.v[i] > value))
            return false;
    return true;
}

GLK_INLINE bool GLKVector4AllGreaterThanOrEqualToVector4(GLKVector4 a, GLKVector4 b)
{
    for (int i = 0; i < 4; i++)
        if (!(a.v[i] >= b.v[i]))
            return false;
    return true;
}

GLK_INLINE bool GLKVector4AllGreaterThanOrEqualToScalar(GLKVector4 a, float value)
{
    for (int i = 0; i < 4; i++)
        if (!(a.v[i] >= value))
            return false;
    return true;
}

GLK_INLINE float GLKVector4DotProduct(GLKVector4 a, GLKVector4 b)
{
    float d = 0.0f;
    for (int i = 0; i < 4; i++)
        d += a.v[i] * b.v[i];
    return d;
}

GLK_INLINE float GLKVector4Length(GLKVector4 a)
{
    return sqrtf(GLKVector4DotProduct(a, a));
}

GLK_INLINE GLKVector4 GLKVector4Normalize(GLKVector4 a)
{
    return GLKVector4MultiplyScalar(a, 1.0f / GLKVector4Length(a));
}

GLK_INLINE float GLKVector4Distance(GLKVector4 a, GLKVector4 b)
{
    return GLKVector4Length(GLKVector4Subtract(b, a));
}

GLK_INLINE GLKVector4 GLKVector4Lerp(GLKVector4 a, GLKVector4 b, float t)
{
    return GLKVector4Add(a, GLKVector4MultiplyScalar(GLKVector4Subtract(b, a), t));
}

GLK_INLINE GLKVector4 GLKVector4Project(GLKVector4 vectorToProject, GLKVector4 projectionVector)
{
    float scale = GLKVector4DotProduct(projectionVector, vectorToProject) / GLKVector4DotProduct(projectionVector, projectionVector);
    return GLKVector4MultiplyScalar(projectionVector, scale);
}

GLK_INLINE GLKVector4 GLKVector4CrossProduct(GLKVector4 a, GLKVector4 b)
{
    GLKVector4 r = { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x, 0.0f };
    return r;
}

#ifdef __cplusplus
}
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
