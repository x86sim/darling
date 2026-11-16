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

#ifndef SIMD_BASE_H
#define SIMD_BASE_H

/*
 * Common attributes for the inline simd functions. In C every function
 * family is overloaded on its argument types, which is how Swift imports
 * them as one function per type; C++ overloads natively.
 */
#ifdef __cplusplus
#define SIMD_OVERLOAD
#else
#define SIMD_OVERLOAD __attribute__((__overloadable__))
#endif

#define SIMD_FN static inline __attribute__((__always_inline__, __unused__)) SIMD_OVERLOAD
#define SIMD_FN_NOOVERLOAD static inline __attribute__((__always_inline__, __unused__))

#endif
