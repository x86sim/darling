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


// GLKit math: known-value checks of the vector, matrix and quaternion functions, from the
// documented definitions (column-major matrices, right-handed OpenGL conventions).
// Prints PASS when everything holds; the exit code is the number of failed checks.

#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#include <GLKit/GLKMath.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)
#define NEAR(a, b) (fabsf((a) - (b)) < 1e-5f)

static int vec3_near(GLKVector3 v, float x, float y, float z) { return NEAR(v.x, x) && NEAR(v.y, y) && NEAR(v.z, z); }
static int vec4_near(GLKVector4 v, float x, float y, float z, float w) { return NEAR(v.x, x) && NEAR(v.y, y) && NEAR(v.z, z) && NEAR(v.w, w); }
static int mat4_near(GLKMatrix4 a, GLKMatrix4 b) { for (int i = 0; i < 16; i++) if (!NEAR(a.m[i], b.m[i])) return 0; return 1; }
static int mat3_near(GLKMatrix3 a, GLKMatrix3 b) { for (int i = 0; i < 9; i++) if (!NEAR(a.m[i], b.m[i])) return 0; return 1; }

static void test_layout(void)
{
    CHECK(sizeof(GLKMatrix2) == 16 && sizeof(GLKMatrix3) == 36 && sizeof(GLKMatrix4) == 64);
    CHECK(sizeof(GLKVector2) == 8 && sizeof(GLKVector3) == 12 && sizeof(GLKVector4) == 16);
    CHECK(sizeof(GLKQuaternion) == 16);
    CHECK(_Alignof(GLKMatrix4) == 16 && _Alignof(GLKVector4) == 16 && _Alignof(GLKQuaternion) == 16);
    CHECK(_Alignof(GLKVector2) == 8 && _Alignof(GLKVector3) == 4 && _Alignof(GLKMatrix3) == 4);

    GLKMatrix4 a = GLKMatrix4Make(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    CHECK(a.m01 == 2 && a.m10 == 5 && a.m[1] == 2 && a.m[4] == 5 && a.m33 == 16);
    GLKMatrix4 t = GLKMatrix4MakeAndTranspose(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    CHECK(t.m[1] == 5 && t.m[4] == 2);
    GLKQuaternion q = GLKQuaternionMake(1, 2, 3, 4);
    CHECK(q.x == 1 && q.w == 4 && q.v.z == 3 && q.s == 4 && q.q[3] == 4);
    GLKVector4 v = GLKVector4Make(1, 2, 3, 4);
    CHECK(v.r == 1 && v.a == 4 && v.s == 1 && v.q == 4 && v.v[2] == 3);
    GLKVector3 w = GLKVector3Make(1, 2, 3);
    CHECK(w.r == 1 && w.b == 3 && w.s == 1 && w.p == 3);
}

static void test_vectors(void)
{
    GLKVector3 a = GLKVector3Make(1, 2, 3), b = GLKVector3Make(4, 5, 6);
    CHECK(vec3_near(GLKVector3Add(a, b), 5, 7, 9));
    CHECK(vec3_near(GLKVector3Subtract(b, a), 3, 3, 3));
    CHECK(vec3_near(GLKVector3Multiply(a, b), 4, 10, 18));
    CHECK(vec3_near(GLKVector3Divide(b, a), 4, 2.5f, 2));
    CHECK(vec3_near(GLKVector3Negate(a), -1, -2, -3));
    CHECK(vec3_near(GLKVector3AddScalar(a, 1), 2, 3, 4));
    CHECK(vec3_near(GLKVector3MultiplyScalar(a, 2), 2, 4, 6));
    CHECK(vec3_near(GLKVector3DivideScalar(b, 2), 2, 2.5f, 3));
    CHECK(NEAR(GLKVector3DotProduct(a, b), 32));
    CHECK(vec3_near(GLKVector3CrossProduct(a, b), -3, 6, -3));
    CHECK(vec3_near(GLKVector3CrossProduct(GLKVector3Make(1, 0, 0), GLKVector3Make(0, 1, 0)), 0, 0, 1));
    CHECK(NEAR(GLKVector3Length(GLKVector3Make(2, 3, 6)), 7));
    CHECK(vec3_near(GLKVector3Normalize(GLKVector3Make(0, 3, 4)), 0, 0.6f, 0.8f));
    CHECK(NEAR(GLKVector3Distance(a, GLKVector3Make(1, 2, 7)), 4));
    CHECK(vec3_near(GLKVector3Lerp(a, b, 0.5f), 2.5f, 3.5f, 4.5f));
    CHECK(vec3_near(GLKVector3Maximum(GLKVector3Make(1, 5, 3), GLKVector3Make(4, 2, 6)), 4, 5, 6));
    CHECK(vec3_near(GLKVector3Minimum(GLKVector3Make(1, 5, 3), GLKVector3Make(4, 2, 6)), 1, 2, 3));
    CHECK(vec3_near(GLKVector3Project(GLKVector3Make(2, 3, 0), GLKVector3Make(4, 0, 0)), 2, 0, 0));
    CHECK(GLKVector3AllEqualToVector3(a, GLKVector3Make(1, 2, 3)) && !GLKVector3AllEqualToVector3(a, b));
    CHECK(GLKVector3AllEqualToScalar(GLKVector3Make(2, 2, 2), 2));
    CHECK(GLKVector3AllGreaterThanVector3(b, a) && !GLKVector3AllGreaterThanVector3(a, a));
    CHECK(GLKVector3AllGreaterThanOrEqualToVector3(a, a));
    CHECK(GLKVector3AllGreaterThanScalar(b, 3) && !GLKVector3AllGreaterThanScalar(b, 4));
    CHECK(GLKVector3AllGreaterThanOrEqualToScalar(b, 4));

    float arr[4] = { 7, 8, 9, 10 };
    CHECK(vec3_near(GLKVector3MakeWithArray(arr), 7, 8, 9));
    CHECK(vec4_near(GLKVector4MakeWithArray(arr), 7, 8, 9, 10));

    GLKVector2 p = GLKVector2Make(3, 4);
    CHECK(NEAR(GLKVector2Length(p), 5) && NEAR(GLKVector2DotProduct(p, GLKVector2Make(1, 2)), 11));
    CHECK(NEAR(GLKVector2Add(p, p).y, 8) && NEAR(GLKVector2Normalize(p).x, 0.6f));
    CHECK(NEAR(GLKVector2Distance(p, GLKVector2Make(0, 0)), 5));

    GLKVector4 c = GLKVector4CrossProduct(GLKVector4Make(1, 0, 0, 5), GLKVector4Make(0, 1, 0, 7));
    CHECK(vec4_near(c, 0, 0, 1, 0));
    CHECK(NEAR(GLKVector4Length(GLKVector4Make(1, 1, 1, 1)), 2));
    CHECK(vec4_near(GLKVector4Lerp(GLKVector4Make(0, 0, 0, 0), GLKVector4Make(2, 4, 6, 8), 0.25f), 0.5f, 1, 1.5f, 2));
}

static void test_matrix4(void)
{
    GLKMatrix4 id = GLKMatrix4Identity;
    CHECK(id.m00 == 1 && id.m11 == 1 && id.m22 == 1 && id.m33 == 1 && id.m01 == 0 && id.m30 == 0);

    GLKMatrix4 tr = GLKMatrix4MakeTranslation(1, 2, 3);
    CHECK(tr.m30 == 1 && tr.m31 == 2 && tr.m32 == 3 && tr.m33 == 1);
    CHECK(vec3_near(GLKMatrix4MultiplyVector3WithTranslation(tr, GLKVector3Make(1, 1, 1)), 2, 3, 4));
    CHECK(vec3_near(GLKMatrix4MultiplyVector3(tr, GLKVector3Make(1, 1, 1)), 1, 1, 1));
    CHECK(vec4_near(GLKMatrix4MultiplyVector4(tr, GLKVector4Make(1, 1, 1, 1)), 2, 3, 4, 1));

    GLKMatrix4 sc = GLKMatrix4MakeScale(2, 3, 4);
    CHECK(vec3_near(GLKMatrix4MultiplyVector3(sc, GLKVector3Make(1, 1, 1)), 2, 3, 4));

    float quarter = (float)M_PI / 2;
    CHECK(vec3_near(GLKMatrix4MultiplyVector3(GLKMatrix4MakeXRotation(quarter), GLKVector3Make(0, 1, 0)), 0, 0, 1));
    CHECK(vec3_near(GLKMatrix4MultiplyVector3(GLKMatrix4MakeYRotation(quarter), GLKVector3Make(0, 0, 1)), 1, 0, 0));
    CHECK(vec3_near(GLKMatrix4MultiplyVector3(GLKMatrix4MakeZRotation(quarter), GLKVector3Make(1, 0, 0)), 0, 1, 0));
    CHECK(mat4_near(GLKMatrix4MakeRotation(quarter, 0, 0, 1), GLKMatrix4MakeZRotation(quarter)));
    CHECK(mat4_near(GLKMatrix4MakeRotation(quarter, 0, 0, 5), GLKMatrix4MakeZRotation(quarter)));
    CHECK(mat4_near(GLKMatrix4MakeRotation(quarter, 1, 0, 0), GLKMatrix4MakeXRotation(quarter)));
    CHECK(mat4_near(GLKMatrix4MakeRotation(quarter, 0, 1, 0), GLKMatrix4MakeYRotation(quarter)));

    // Multiply(l, r) applies r first.
    GLKMatrix4 m = GLKMatrix4Multiply(tr, GLKMatrix4MakeZRotation(quarter));
    CHECK(vec3_near(GLKMatrix4MultiplyVector3WithTranslation(m, GLKVector3Make(1, 0, 0)), 1, 3, 3));
    // Translate/Scale/Rotate post-multiply.
    CHECK(mat4_near(GLKMatrix4Translate(sc, 1, 2, 3), GLKMatrix4Multiply(sc, tr)));
    CHECK(mat4_near(GLKMatrix4Scale(tr, 2, 3, 4), GLKMatrix4Multiply(tr, sc)));
    CHECK(mat4_near(GLKMatrix4RotateZ(tr, quarter), m));
    CHECK(mat4_near(GLKMatrix4Rotate(tr, quarter, 0, 0, 1), m));
    CHECK(mat4_near(GLKMatrix4RotateWithVector3(tr, quarter, GLKVector3Make(0, 0, 1)), m));
    CHECK(mat4_near(GLKMatrix4TranslateWithVector3(sc, GLKVector3Make(1, 2, 3)), GLKMatrix4Multiply(sc, tr)));
    CHECK(mat4_near(GLKMatrix4ScaleWithVector3(tr, GLKVector3Make(2, 3, 4)), GLKMatrix4Multiply(tr, sc)));

    GLKMatrix4 sum = GLKMatrix4Add(id, id);
    CHECK(sum.m00 == 2 && sum.m01 == 0);
    CHECK(mat4_near(GLKMatrix4Subtract(sum, id), id));

    GLKMatrix4 a = GLKMatrix4Make(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    GLKMatrix4 at = GLKMatrix4Transpose(a);
    CHECK(at.m01 == 5 && at.m10 == 2 && at.m33 == 16);
    CHECK(vec4_near(GLKMatrix4GetRow(a, 1), 2, 6, 10, 14));
    CHECK(vec4_near(GLKMatrix4GetColumn(a, 1), 5, 6, 7, 8));
    GLKMatrix4 s = GLKMatrix4SetRow(id, 2, GLKVector4Make(1, 2, 3, 4));
    CHECK(s.m02 == 1 && s.m12 == 2 && s.m22 == 3 && s.m32 == 4);
    s = GLKMatrix4SetColumn(id, 3, GLKVector4Make(1, 2, 3, 4));
    CHECK(s.m30 == 1 && s.m31 == 2 && s.m32 == 3 && s.m33 == 4);
    GLKMatrix4 rows = GLKMatrix4MakeWithRows(GLKVector4Make(1, 2, 3, 4), GLKVector4Make(5, 6, 7, 8), GLKVector4Make(9, 10, 11, 12), GLKVector4Make(13, 14, 15, 16));
    CHECK(mat4_near(rows, at));
    GLKMatrix4 cols = GLKMatrix4MakeWithColumns(GLKVector4Make(1, 2, 3, 4), GLKVector4Make(5, 6, 7, 8), GLKVector4Make(9, 10, 11, 12), GLKVector4Make(13, 14, 15, 16));
    CHECK(mat4_near(cols, a));
    float arr[16];
    for (int i = 0; i < 16; i++) arr[i] = (float)(i + 1);
    CHECK(mat4_near(GLKMatrix4MakeWithArray(arr), a));
    CHECK(mat4_near(GLKMatrix4MakeWithArrayAndTranspose(arr), at));
    GLKMatrix3 a3 = GLKMatrix4GetMatrix3(a);
    CHECK(a3.m00 == 1 && a3.m02 == 3 && a3.m10 == 5 && a3.m22 == 11);
    GLKMatrix2 a2 = GLKMatrix4GetMatrix2(a);
    CHECK(a2.m00 == 1 && a2.m01 == 2 && a2.m10 == 5 && a2.m11 == 6);

    // Projection matrices (OpenGL conventions).
    GLKMatrix4 persp = GLKMatrix4MakePerspective((float)M_PI / 2, 2.0f, 1.0f, 3.0f);
    CHECK(NEAR(persp.m00, 0.5f) && NEAR(persp.m11, 1.0f) && NEAR(persp.m22, -2.0f) && NEAR(persp.m23, -1.0f) && NEAR(persp.m32, -3.0f) && NEAR(persp.m33, 0.0f));
    GLKMatrix4 ortho = GLKMatrix4MakeOrtho(0, 4, 0, 2, 1, 3);
    CHECK(NEAR(ortho.m00, 0.5f) && NEAR(ortho.m11, 1.0f) && NEAR(ortho.m22, -1.0f) && NEAR(ortho.m30, -1.0f) && NEAR(ortho.m31, -1.0f) && NEAR(ortho.m32, -2.0f) && NEAR(ortho.m33, 1.0f));
    GLKMatrix4 fr = GLKMatrix4MakeFrustum(-1, 1, -1, 1, 1, 3);
    CHECK(NEAR(fr.m00, 1.0f) && NEAR(fr.m11, 1.0f) && NEAR(fr.m22, -2.0f) && NEAR(fr.m23, -1.0f) && NEAR(fr.m32, -3.0f) && NEAR(fr.m20, 0.0f));
    GLKMatrix4 look = GLKMatrix4MakeLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
    CHECK(mat4_near(look, GLKMatrix4MakeTranslation(0, 0, -5)));
    look = GLKMatrix4MakeLookAt(5, 0, 0, 0, 0, 0, 0, 1, 0);
    CHECK(vec3_near(GLKMatrix4MultiplyVector3WithTranslation(look, GLKVector3Make(0, 0, 0)), 0, 0, -5));
    CHECK(vec3_near(GLKMatrix4MultiplyVector3WithTranslation(look, GLKVector3Make(5, 0, 0)), 0, 0, 0));
    CHECK(vec3_near(GLKMatrix4MultiplyVector3(look, GLKVector3Make(0, 0, -1)), 1, 0, 0));

    // Inverse: M * inverse(M) is the identity; a singular matrix returns the identity and reports it.
    bool ok = false;
    GLKMatrix4 general = GLKMatrix4Multiply(GLKMatrix4Multiply(tr, GLKMatrix4MakeRotation(0.7f, 1, 2, 3)), sc);
    GLKMatrix4 inv = GLKMatrix4Invert(general, &ok);
    CHECK(ok && mat4_near(GLKMatrix4Multiply(general, inv), id));
    CHECK(mat4_near(GLKMatrix4InvertAndTranspose(general, &ok), GLKMatrix4Transpose(inv)) && ok);
    CHECK(mat4_near(GLKMatrix4Invert(tr, NULL), GLKMatrix4MakeTranslation(-1, -2, -3)));
    ok = true;
    CHECK(mat4_near(GLKMatrix4Invert(a, &ok), id) && !ok);
    ok = true;
    CHECK(mat4_near(GLKMatrix4Invert(GLKMatrix4Make(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), &ok), id) && !ok);

    GLKVector3 pts[2] = { { 1, 0, 0 }, { 0, 1, 0 } };
    GLKMatrix4MultiplyVector3ArrayWithTranslation(tr, pts, 2);
    CHECK(vec3_near(pts[0], 2, 2, 3) && vec3_near(pts[1], 1, 3, 3));
    GLKMatrix4MultiplyVector3Array(sc, pts, 2);
    CHECK(vec3_near(pts[0], 4, 6, 12));
    GLKVector4 p4[1] = { { 1, 1, 1, 1 } };
    GLKMatrix4MultiplyVector4Array(tr, p4, 1);
    CHECK(vec4_near(p4[0], 2, 3, 4, 1));
    CHECK(vec3_near(GLKMatrix4MultiplyAndProjectVector3(persp, GLKVector3Make(0, 0, -2)), 0, 0, 0.5f));
}

static void test_matrix3_2(void)
{
    GLKMatrix3 id = GLKMatrix3Identity;
    CHECK(id.m00 == 1 && id.m11 == 1 && id.m22 == 1 && id.m01 == 0);
    GLKMatrix3 a = GLKMatrix3Make(1, 2, 3, 4, 5, 6, 7, 8, 10);
    CHECK(a.m01 == 2 && a.m10 == 4 && a.m22 == 10);
    CHECK(GLKMatrix3Transpose(a).m01 == 4);
    bool ok = false;
    GLKMatrix3 inv = GLKMatrix3Invert(a, &ok);
    CHECK(ok && mat3_near(GLKMatrix3Multiply(a, inv), id));
    // det of this matrix is -3: inverse of column-major (1,2,3 | 4,5,6 | 7,8,10)
    CHECK(NEAR(inv.m00, -2.0f / 3.0f) && NEAR(inv.m01, -4.0f / 3.0f) && NEAR(inv.m02, 1.0f));
    ok = true;
    CHECK(mat3_near(GLKMatrix3Invert(GLKMatrix3Make(1, 2, 3, 4, 5, 6, 7, 8, 9), &ok), id) && !ok);
    CHECK(mat3_near(GLKMatrix3InvertAndTranspose(a, NULL), GLKMatrix3Transpose(inv)));
    CHECK(vec3_near(GLKMatrix3MultiplyVector3(GLKMatrix3MakeScale(2, 3, 4), GLKVector3Make(1, 1, 1)), 2, 3, 4));
    CHECK(vec3_near(GLKMatrix3MultiplyVector3(GLKMatrix3MakeZRotation((float)M_PI / 2), GLKVector3Make(1, 0, 0)), 0, 1, 0));
    CHECK(vec3_near(GLKMatrix3MultiplyVector3(GLKMatrix3MakeRotation((float)M_PI / 2, 1, 0, 0), GLKVector3Make(0, 1, 0)), 0, 0, 1));
    CHECK(vec3_near(GLKMatrix3GetRow(a, 0), 1, 4, 7) && vec3_near(GLKMatrix3GetColumn(a, 2), 7, 8, 10));
    CHECK(mat3_near(GLKMatrix3Add(id, id), GLKMatrix3Scale(id, 2, 2, 2)));
    GLKMatrix2 a2 = GLKMatrix3GetMatrix2(a);
    CHECK(a2.m00 == 1 && a2.m01 == 2 && a2.m10 == 4 && a2.m11 == 5);
}

static void test_quaternion(void)
{
    GLKQuaternion id = GLKQuaternionIdentity;
    CHECK(id.x == 0 && id.y == 0 && id.z == 0 && id.w == 1);
    float half = (float)M_PI / 2;
    GLKQuaternion qz = GLKQuaternionMakeWithAngleAndAxis(half, 0, 0, 1);
    float s = sqrtf(0.5f);
    CHECK(NEAR(qz.x, 0) && NEAR(qz.y, 0) && NEAR(qz.z, s) && NEAR(qz.w, s));
    CHECK(NEAR(GLKQuaternionLength(qz), 1));
    CHECK(vec3_near(GLKQuaternionRotateVector3(qz, GLKVector3Make(1, 0, 0)), 0, 1, 0));
    CHECK(vec4_near(GLKQuaternionRotateVector4(qz, GLKVector4Make(1, 0, 0, 1)), 0, 1, 0, 1));
    GLKQuaternion qv = GLKQuaternionMakeWithAngleAndVector3Axis(half, GLKVector3Make(0, 0, 3));
    CHECK(NEAR(qv.z, 3 * s) && NEAR(qv.w, s)); // test_quirk_angle_axis_not_normalized
    qv = GLKQuaternionMakeWithAngleAndAxis(half, 0, 0, 3);
    CHECK(NEAR(qv.z, 3 * s) && NEAR(qv.w, s)); // test_quirk_angle_axis_not_normalized
    CHECK(NEAR(GLKQuaternionAngle(qz), half));
    CHECK(vec3_near(GLKQuaternionAxis(qz), 0, 0, 1));

    GLKQuaternion c = GLKQuaternionConjugate(GLKQuaternionMake(1, 2, 3, 4));
    CHECK(c.x == -1 && c.y == -2 && c.z == -3 && c.w == 4);
    GLKQuaternion inv = GLKQuaternionInvert(GLKQuaternionMake(1, 2, 3, 4));
    CHECK(NEAR(inv.x, -1.0f / 30) && NEAR(inv.w, 4.0f / 30));
    GLKQuaternion prod = GLKQuaternionMultiply(GLKQuaternionMake(1, 2, 3, 4), GLKQuaternionMake(5, 6, 7, 8));
    CHECK(NEAR(prod.x, 24) && NEAR(prod.y, 48) && NEAR(prod.z, 48) && NEAR(prod.w, -6));
    CHECK(NEAR(GLKQuaternionAdd(qz, qz).w, 2 * s) && NEAR(GLKQuaternionSubtract(qz, qz).z, 0));
    CHECK(NEAR(GLKQuaternionLength(GLKQuaternionNormalize(GLKQuaternionMake(1, 2, 3, 4))), 1));
    GLKQuaternion mv = GLKQuaternionMakeWithVector3(GLKVector3Make(1, 2, 3), 4);
    CHECK(mv.x == 1 && mv.z == 3 && mv.w == 4);
    float arr[4] = { 1, 2, 3, 4 };
    CHECK(GLKQuaternionMakeWithArray(arr).w == 4);

    GLKQuaternion qh = GLKQuaternionSlerp(id, qz, 0.5f);
    GLKQuaternion q45 = GLKQuaternionMakeWithAngleAndAxis(half / 2, 0, 0, 1);
    CHECK(NEAR(qh.z, q45.z) && NEAR(qh.w, q45.w));
    GLKQuaternion q0 = GLKQuaternionSlerp(id, qz, 0), q1 = GLKQuaternionSlerp(id, qz, 1);
    CHECK(NEAR(q0.w, 1) && NEAR(q1.z, s));

    GLKVector3 pts[2] = { { 1, 0, 0 }, { 0, 1, 0 } };
    GLKQuaternionRotateVector3Array(qz, pts, 2);
    CHECK(vec3_near(pts[0], 0, 1, 0) && vec3_near(pts[1], -1, 0, 0));
    GLKVector4 p4[1] = { { 1, 0, 0, 1 } };
    GLKQuaternionRotateVector4Array(qz, p4, 1);
    CHECK(vec4_near(p4[0], 0, 1, 0, 1));

    // Quaternion <-> rotation matrix, in both directions.
    GLKMatrix3 m3 = GLKMatrix3MakeWithQuaternion(qz);
    CHECK(mat3_near(m3, GLKMatrix3MakeZRotation(half)));
    CHECK(mat4_near(GLKMatrix4MakeWithQuaternion(qz), GLKMatrix4MakeZRotation(half)));
    GLKQuaternion back = GLKQuaternionMakeWithMatrix3(m3);
    CHECK(NEAR(back.z, s) && NEAR(back.w, s));
    GLKQuaternion odd = GLKQuaternionMakeWithAngleAndAxis(3.0f, 1.0f / 3, 2.0f / 3, 2.0f / 3);
    GLKQuaternion back2 = GLKQuaternionMakeWithMatrix4(GLKMatrix4MakeWithQuaternion(odd));
    float d = back2.x * odd.x + back2.y * odd.y + back2.z * odd.z + back2.w * odd.w;
    CHECK(NEAR(fabsf(d), 1));
    // pivot on each diagonal element
    GLKQuaternion qx = GLKQuaternionMakeWithAngleAndAxis(3.0f, 1, 0, 0);
    GLKQuaternion qy = GLKQuaternionMakeWithAngleAndAxis(3.0f, 0, 1, 0);
    GLKQuaternion qzz = GLKQuaternionMakeWithAngleAndAxis(3.0f, 0, 0, 1);
    GLKQuaternion arr3[3] = { qx, qy, qzz };
    for (int i = 0; i < 3; i++)
    {
        GLKQuaternion r = GLKQuaternionMakeWithMatrix3(GLKMatrix3MakeWithQuaternion(arr3[i]));
        float dd = r.x * arr3[i].x + r.y * arr3[i].y + r.z * arr3[i].z + r.w * arr3[i].w;
        CHECK(NEAR(fabsf(dd), 1));
    }
}

static void test_utils(void)
{
    CHECK(NEAR(GLKMathDegreesToRadians(180), (float)M_PI) && NEAR(GLKMathRadiansToDegrees((float)M_PI / 2), 90));

    int viewport[4] = { 0, 0, 200, 100 };
    GLKMatrix4 proj = GLKMatrix4MakeOrtho(-2, 2, -1, 1, 1, 3);
    GLKMatrix4 model = GLKMatrix4MakeTranslation(0, 0, -2);
    GLKVector3 win = GLKMathProject(GLKVector3Make(0, 0, 0), model, proj, viewport);
    CHECK(vec3_near(win, 100, 50, 0.5f));
    win = GLKMathProject(GLKVector3Make(2, 1, 0), model, proj, viewport);
    CHECK(vec3_near(win, 200, 100, 0.5f));
    bool ok = false;
    GLKVector3 obj = GLKMathUnproject(GLKVector3Make(150, 25, 0.5f), model, proj, viewport, &ok);
    CHECK(ok && vec3_near(obj, 1, -0.5f, 0));
    ok = true;
    GLKMathUnproject(GLKVector3Make(1, 1, 0), GLKMatrix4MakeScale(0, 1, 1), proj, viewport, &ok);
    CHECK(!ok);
    obj = GLKMathUnproject(GLKVector3Make(100, 50, 0.5f), model, proj, viewport, NULL);
    CHECK(vec3_near(obj, 0, 0, 0));
}

// Undocumented behaviour observed on macOS 26 (see the QUIRK comments in GLKMatrix.m / GLKQuaternion.h).
static void test_quirks(void)
{
    // test_quirk_quaternion_angle_axis_edge_cases: input x y z w -> axis x y z, angle
    static const float angle_axis[][8] = {
        { 0, 0, 1, 0,       0, 0, 1,                  3.14159f },
        { 0, 0, 2, 0.5f,    0, 0, 1,                  2.09439f },
        { 1, 2, 3, 4,       0.267261f, 0.534522f, 0.801784f, 1.50408f },
        { 1, 2, 2, 0,       1.0f / 3, 2.0f / 3, 2.0f / 3,    3.14159f },
        { 0, 0, 0, 1,       0, 0, 0,                  0 },
        { 0, 0, 0, -1,      0, 0, 0,                  6.28319f },
        { 0, 0, 1, -0.5f,   0, 0, 1,                  4.18879f },
        { 0, 3, 4, 0.5f,    0, 0.6f, 0.8f,            2.09439f },
        { 0, 0, 0, 2,       0, 0, 0,                  0 },
        { 0, 0, 0, -2,      0, 0, 0,                  6.28319f },
        { 0, 0, 0.5f, 0.9f, 0, 0, 1,                  0.902054f },
    };
    for (unsigned i = 0; i < sizeof(angle_axis) / sizeof(angle_axis[0]); i++)
    {
        const float *e = angle_axis[i];
        GLKQuaternion q = GLKQuaternionMake(e[0], e[1], e[2], e[3]);
        CHECK(vec3_near(GLKQuaternionAxis(q), e[4], e[5], e[6]));
        CHECK(fabsf(GLKQuaternionAngle(q) - e[7]) < 1e-4f);
    }
    GLKQuaternion zero = GLKQuaternionMake(0, 0, 0, 0);
    CHECK(isnan(GLKQuaternionAxis(zero).x) && fabsf(GLKQuaternionAngle(zero) - 3.14159f) < 1e-4f);

    // test_quirk_slerp_not_normalized
    float c = cosf(1.5f), sn = sinf(1.5f);
    GLKQuaternion r = GLKQuaternionSlerp(GLKQuaternionIdentity, GLKQuaternionMake(0, 0, 2 * sn, 2 * c), 0.5f);
    CHECK(fabsf(r.z - 1.32036f) < 1e-4f && fabsf(r.w - 0.755472f) < 1e-4f);
    GLKQuaternion neg = GLKQuaternionSlerp(GLKQuaternionIdentity, GLKQuaternionMake(0, 0, -sn, -c), 0.5f);
    CHECK(fabsf(neg.z - 0.681639f) < 1e-4f);
    GLKQuaternion ex = GLKQuaternionSlerp(GLKQuaternionIdentity, GLKQuaternionMake(0, 0, sn, c), 1.5f);
    CHECK(fabsf(ex.z - 0.778073f) < 1e-4f && fabsf(ex.w + 0.628174f) < 1e-4f);

    // test_quirk_quaternion_from_matrix_not_normalized
    GLKQuaternion qm = GLKQuaternionMakeWithMatrix3(GLKMatrix3Make(0, 0, 0, 0, 0, 0, 0, 0, 0));
    CHECK(NEAR(qm.x, 0) && NEAR(qm.y, 0) && NEAR(qm.z, 0) && NEAR(qm.w, 0.5f));
    qm = GLKQuaternionMakeWithMatrix3(GLKMatrix3MakeScale(2, 2, 2));
    CHECK(fabsf(qm.w - 1.32288f) < 1e-4f && NEAR(qm.x, 0));
    qm = GLKQuaternionMakeWithMatrix3(GLKMatrix3MakeXRotation((float)M_PI));
    CHECK(NEAR(qm.x, 1) && fabsf(qm.w) < 1e-6f);
    qm = GLKQuaternionMakeWithMatrix3(GLKMatrix3MakeRotation(2.0943951f, 1, 1, 1));
    CHECK(NEAR(qm.x, 0.5f) && NEAR(qm.y, 0.5f) && NEAR(qm.z, 0.5f) && NEAR(qm.w, 0.5f));

    // test_quirk_unproject_singular_uses_identity
    int viewport[4] = { 0, 0, 100, 50 };
    bool ok = true;
    GLKVector3 u = GLKMathUnproject(GLKVector3Make(1, 1, 1), GLKMatrix4MakeScale(0, 1, 1), GLKMatrix4Identity, viewport, &ok);
    CHECK(!ok && vec3_near(u, -0.98f, -0.96f, 1.0f));

    // Documented fall-backs seen alongside: singular inverse is the identity, zero vectors normalize to NaN.
    CHECK(isnan(GLKVector3Normalize(GLKVector3Make(0, 0, 0)).x));
    CHECK(isnan(GLKQuaternionNormalize(GLKQuaternionMake(0, 0, 0, 0)).w));
    CHECK(isnan(GLKMatrix4MakeRotation(1, 0, 0, 0).m00) && GLKMatrix4MakeRotation(1, 0, 0, 0).m33 == 1);
}

int main(void)
{
    test_layout();
    test_vectors();
    test_matrix4();
    test_matrix3_2();
    test_quaternion();
    test_utils();
    test_quirks();
    if (failures == 0)
        printf("PASS\n");
    return failures;
}
