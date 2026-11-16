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


#ifndef _GLKMathUtils_H_
#define _GLKMathUtils_H_

#include <GLKit/GLKMathTypes.h>
#include <GLKit/GLKVector3.h>
#include <GLKit/GLKMatrix4.h>

#ifdef __cplusplus
extern "C" {
#endif

GLK_INLINE float GLKMathDegreesToRadians(float degrees)
{
    return degrees * ((float)M_PI / 180.0f);
}

GLK_INLINE float GLKMathRadiansToDegrees(float radians)
{
    return radians * (180.0f / (float)M_PI);
}

// Maps an object-space point to window coordinates (viewport is x, y, width, height).
extern GLKVector3 GLKMathProject(GLKVector3 object, GLKMatrix4 model, GLKMatrix4 projection, int *viewport);

// Inverse of GLKMathProject; *success (when not NULL) tells whether the matrices could be inverted.
extern GLKVector3 GLKMathUnproject(GLKVector3 window, GLKMatrix4 model, GLKMatrix4 projection, int *viewport, bool *success);

#ifdef __cplusplus
}
#endif

#endif
