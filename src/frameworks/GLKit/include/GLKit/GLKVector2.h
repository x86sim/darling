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


#ifndef _GLKVector2_H_
#define _GLKVector2_H_

#include <GLKit/GLKMathTypes.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#ifdef __cplusplus
extern "C" {
#endif

GLK_INLINE GLKVector2 GLKVector2Make(float x, float y)
{
    GLKVector2 v = { x, y };
    return v;
}

GLK_INLINE GLKVector2 GLKVector2MakeWithArray(float values[2])
{
    GLKVector2 v;
    for (int i = 0; i < 2; i++)
        v.v[i] = values[i];
    return v;
}

GLK_INLINE GLKVector2 GLKVector2Add(GLKVector2 a, GLKVector2 b)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = a.v[i] + b.v[i];
    return r;
}

GLK_INLINE GLKVector2 GLKVector2Subtract(GLKVector2 a, GLKVector2 b)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = a.v[i] - b.v[i];
    return r;
}

GLK_INLINE GLKVector2 GLKVector2Multiply(GLKVector2 a, GLKVector2 b)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = a.v[i] * b.v[i];
    return r;
}

GLK_INLINE GLKVector2 GLKVector2Divide(GLKVector2 a, GLKVector2 b)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = a.v[i] / b.v[i];
    return r;
}

GLK_INLINE GLKVector2 GLKVector2Negate(GLKVector2 a)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = -a.v[i];
    return r;
}

GLK_INLINE GLKVector2 GLKVector2AddScalar(GLKVector2 a, float value)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = a.v[i] + value;
    return r;
}

GLK_INLINE GLKVector2 GLKVector2SubtractScalar(GLKVector2 a, float value)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = a.v[i] - value;
    return r;
}

GLK_INLINE GLKVector2 GLKVector2MultiplyScalar(GLKVector2 a, float value)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = a.v[i] * value;
    return r;
}

GLK_INLINE GLKVector2 GLKVector2DivideScalar(GLKVector2 a, float value)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = a.v[i] / value;
    return r;
}

GLK_INLINE GLKVector2 GLKVector2Maximum(GLKVector2 a, GLKVector2 b)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = a.v[i] > b.v[i] ? a.v[i] : b.v[i];
    return r;
}

GLK_INLINE GLKVector2 GLKVector2Minimum(GLKVector2 a, GLKVector2 b)
{
    GLKVector2 r;
    for (int i = 0; i < 2; i++)
        r.v[i] = a.v[i] < b.v[i] ? a.v[i] : b.v[i];
    return r;
}

GLK_INLINE bool GLKVector2AllEqualToVector2(GLKVector2 a, GLKVector2 b)
{
    for (int i = 0; i < 2; i++)
        if (!(a.v[i] == b.v[i]))
            return false;
    return true;
}

GLK_INLINE bool GLKVector2AllEqualToScalar(GLKVector2 a, float value)
{
    for (int i = 0; i < 2; i++)
        if (!(a.v[i] == value))
            return false;
    return true;
}

GLK_INLINE bool GLKVector2AllGreaterThanVector2(GLKVector2 a, GLKVector2 b)
{
    for (int i = 0; i < 2; i++)
        if (!(a.v[i] > b.v[i]))
            return false;
    return true;
}

GLK_INLINE bool GLKVector2AllGreaterThanScalar(GLKVector2 a, float value)
{
    for (int i = 0; i < 2; i++)
        if (!(a.v[i] > value))
            return false;
    return true;
}

GLK_INLINE bool GLKVector2AllGreaterThanOrEqualToVector2(GLKVector2 a, GLKVector2 b)
{
    for (int i = 0; i < 2; i++)
        if (!(a.v[i] >= b.v[i]))
            return false;
    return true;
}

GLK_INLINE bool GLKVector2AllGreaterThanOrEqualToScalar(GLKVector2 a, float value)
{
    for (int i = 0; i < 2; i++)
        if (!(a.v[i] >= value))
            return false;
    return true;
}

GLK_INLINE float GLKVector2DotProduct(GLKVector2 a, GLKVector2 b)
{
    float d = 0.0f;
    for (int i = 0; i < 2; i++)
        d += a.v[i] * b.v[i];
    return d;
}

GLK_INLINE float GLKVector2Length(GLKVector2 a)
{
    return sqrtf(GLKVector2DotProduct(a, a));
}

GLK_INLINE GLKVector2 GLKVector2Normalize(GLKVector2 a)
{
    return GLKVector2MultiplyScalar(a, 1.0f / GLKVector2Length(a));
}

GLK_INLINE float GLKVector2Distance(GLKVector2 a, GLKVector2 b)
{
    return GLKVector2Length(GLKVector2Subtract(b, a));
}

GLK_INLINE GLKVector2 GLKVector2Lerp(GLKVector2 a, GLKVector2 b, float t)
{
    return GLKVector2Add(a, GLKVector2MultiplyScalar(GLKVector2Subtract(b, a), t));
}

GLK_INLINE GLKVector2 GLKVector2Project(GLKVector2 vectorToProject, GLKVector2 projectionVector)
{
    float scale = GLKVector2DotProduct(projectionVector, vectorToProject) / GLKVector2DotProduct(projectionVector, projectionVector);
    return GLKVector2MultiplyScalar(projectionVector, scale);
}

#ifdef __cplusplus
}
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
