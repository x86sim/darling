// Copyright (C) 2026 Lubos Dolezel
//
// <simd/simd.h>: element-wise functions, geometry, matrix products, inverse
// and determinant, identity constants and quaternions, checked against
// values worked out by hand from the documented definitions.
// Prints PASS, or one FAIL line per failed check; the exit code is the
// number of failures.
#include <simd/simd.h>
#include <math.h>
#include <stdio.h>

static int g_failures = 0;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %d: %s\n", __LINE__, #cond); g_failures++; } } while (0)

static int near_f(float a, float b) { return fabsf(a - b) < 1e-5f; }
static int near_d(double a, double b) { return fabs(a - b) < 1e-12; }
static int near3(simd_float3 a, simd_float3 b) { return near_f(a.x, b.x) && near_f(a.y, b.y) && near_f(a.z, b.z); }

static void test_elementwise(void)
{
	simd_float3 v = {-1.5f, 0.0f, 2.25f};
	simd_float3 a = simd_abs(v);
	CHECK(near3(a, ((simd_float3){1.5f, 0.0f, 2.25f})));
	CHECK(near3(simd_sign(v), ((simd_float3){-1, 0, 1})));
	CHECK(near3(simd_min(v, (simd_float3){0, 0, 0}), ((simd_float3){-1.5f, 0, 0})));
	CHECK(near3(simd_max(v, (simd_float3){0, 0, 0}), ((simd_float3){0, 0, 2.25f})));
	CHECK(near3(simd_clamp(v, (simd_float3){-1, -1, -1}, (simd_float3){1, 1, 1}), ((simd_float3){-1, 0, 1})));
	CHECK(near_f(simd_reduce_add(v), 0.75f));
	CHECK(near_f(simd_reduce_min(v), -1.5f));
	CHECK(near_f(simd_reduce_max(v), 2.25f));
	CHECK(near3(simd_recip((simd_float3){2, 4, 0.5f}), ((simd_float3){0.5f, 0.25f, 2})));
	CHECK(near3(simd_rsqrt((simd_float3){4, 16, 1}), ((simd_float3){0.5f, 0.25f, 1})));
	CHECK(near3(simd_fract((simd_float3){1.25f, -0.25f, 3}), ((simd_float3){0.25f, 0.75f, 0})));
	CHECK(near3(simd_step((simd_float3){1, 1, 1}, (simd_float3){0.5f, 1, 2}), ((simd_float3){0, 1, 1})));
	CHECK(near3(simd_smoothstep((simd_float3){0, 0, 0}, (simd_float3){1, 1, 1}, (simd_float3){-1, 0.5f, 2}), ((simd_float3){0, 0.5f, 1})));
	CHECK(near3(simd_mix((simd_float3){0, 0, 0}, (simd_float3){10, 20, 30}, (simd_float3){0.5f, 0.5f, 0.5f}), ((simd_float3){5, 10, 15})));
	simd_int4 iv = {-3, 4, -5, 6};
	simd_int4 ia = simd_abs(iv);
	CHECK(ia.x == 3 && ia.y == 4 && ia.z == 5 && ia.w == 6);
	CHECK(simd_reduce_add(iv) == 2 && simd_reduce_min(iv) == -5 && simd_reduce_max(iv) == 6);
	simd_uint2 uv = {7, 9};
	CHECK(simd_reduce_max(uv) == 9 && simd_reduce_min(uv) == 7);
	CHECK(near_f(simd_sign(-3.0f), -1) && near_f(simd_sign(0.0f), 0) && near_f(simd_sign(2.0f), 1));
	CHECK(near_f(simd_sign(NAN), 0));
	CHECK(near_d(simd_fract(2.75), 0.75) && near_d(simd_clamp(5.0, 0.0, 1.0), 1.0));
}

static void test_geometry(void)
{
	simd_float3 x = {1, 0, 0}, y = {0, 1, 0}, d = {3, 4, 12};
	CHECK(near_f(simd_dot(x, y), 0) && near_f(simd_dot(d, d), 169));
	CHECK(near_f(simd_length(d), 13) && near_f(simd_length_squared(d), 169));
	CHECK(near_f(simd_distance(x, y), sqrtf(2)));
	CHECK(near3(simd_cross(x, y), ((simd_float3){0, 0, 1})));
	CHECK(near_f(simd_cross((simd_float2){1, 0}, (simd_float2){0, 1}), 1));
	CHECK(near3(simd_normalize((simd_float3){0, 3, 4}), ((simd_float3){0, 0.6f, 0.8f})));
	CHECK(near3(simd_project((simd_float3){2, 3, 0}, x), ((simd_float3){2, 0, 0})));
	CHECK(near3(simd_reflect((simd_float3){1, -1, 0}, y), ((simd_float3){1, 1, 0})));
	// eta 1 leaves the direction unchanged; a grazing ray with a high index is totally reflected
	CHECK(near3(simd_refract((simd_float3){0, -1, 0}, y, 1.0f), ((simd_float3){0, -1, 0})));
	CHECK(near3(simd_refract((simd_float3){0.8f, -0.6f, 0}, y, 1.5f), ((simd_float3){0, 0, 0})));
	simd_double4 dd = {1, 2, 2, 4};
	CHECK(near_d(simd_length(dd), 5));
}

static void test_matrix(void)
{
	// 3 columns of 2 rows: [1 3 5; 2 4 6]
	simd_float3x2 m = {{{1, 2}, {3, 4}, {5, 6}}};
	simd_float3 v = {1, 1, 1};
	simd_float2 mv = simd_mul(m, v);
	CHECK(near_f(mv.x, 9) && near_f(mv.y, 12));
	simd_float2 w = {1, 2};
	simd_float3 wm = simd_mul(w, m);
	CHECK(near_f(wm.x, 5) && near_f(wm.y, 11) && near_f(wm.z, 17));
	simd_float2x3 t = simd_transpose(m);
	CHECK(near_f(t.columns[0].x, 1) && near_f(t.columns[0].y, 3) && near_f(t.columns[0].z, 5));
	CHECK(near_f(t.columns[1].x, 2) && near_f(t.columns[1].y, 4) && near_f(t.columns[1].z, 6));
	// (2x3 [1 2; 3 4; 5 6]) is t; t * m is a 3x3 product of a 2-column and 3-column matrix
	simd_float3x3 p = simd_mul(t, m);
	// p = t(3x2 rows) * m(2x3): row0 = [1 2]*m = [5 11 17]
	CHECK(near_f(p.columns[0].x, 5) && near_f(p.columns[1].x, 11) && near_f(p.columns[2].x, 17));
	CHECK(near_f(p.columns[2].z, 61));
	simd_float3x2 s = simd_add(m, m), diff = simd_sub(s, m), sc = simd_mul(2.0f, m);
	CHECK(simd_equal(diff, m) && simd_equal(s, sc) && !simd_equal(s, m));

	simd_float2x2 a = {{{4, 3}, {6, 3}}}; // [4 6; 3 3]
	CHECK(near_f(simd_determinant(a), -6));
	simd_float2x2 ai = simd_inverse(a);
	simd_float2x2 id2 = simd_mul(a, ai);
	CHECK(near_f(id2.columns[0].x, 1) && near_f(id2.columns[1].y, 1));
	CHECK(near_f(id2.columns[0].y, 0) && near_f(id2.columns[1].x, 0));

	simd_float3x3 b = {{{2, 0, 0}, {0, 3, 0}, {1, 1, 4}}};
	CHECK(near_f(simd_determinant(b), 24));
	simd_float3x3 bi = simd_inverse(b), bb = simd_mul(b, bi);
	CHECK(near_f(bb.columns[0].x, 1) && near_f(bb.columns[1].y, 1) && near_f(bb.columns[2].z, 1) && near_f(bb.columns[2].x, 0));

	// 4x4: determinant of a permutation with one swap is -1; scaled diagonal
	simd_float4x4 pm = {{{0, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
	CHECK(near_f(simd_determinant(pm), -1));
	simd_float4x4 dm = simd_diagonal_matrix((simd_float4){2, 3, 4, 5});
	CHECK(near_f(simd_determinant(dm), 120));
	simd_float4x4 gm = {{{1, 2, 0, 1}, {0, 1, 3, 2}, {2, 0, 1, 1}, {1, 1, 1, 0}}};
	simd_float4x4 gi = simd_inverse(gm), gp = simd_mul(gm, gi);
	for (int c = 0; c < 4; c++)
		for (int r = 0; r < 4; r++)
			CHECK(fabsf(gp.columns[c][r] - (c == r ? 1.0f : 0.0f)) < 1e-4f);
	simd_double4x4 dg = {{{1, 2, 0, 1}, {0, 1, 3, 2}, {2, 0, 1, 1}, {1, 1, 1, 0}}};
	CHECK(fabs(simd_determinant(dg) - simd_determinant(gm)) < 1e-4);
	simd_double4x4 dgi = simd_inverse(dg), dgp = simd_mul(dg, dgi);
	CHECK(fabs(dgp.columns[3][3] - 1) < 1e-12 && fabs(dgp.columns[0][1]) < 1e-12);

	// identity constants
	simd_float4x4 i4 = matrix_identity_float4x4;
	CHECK(simd_equal(simd_mul(i4, gm), gm));
	simd_double3x3 i3 = matrix_identity_double3x3;
	CHECK(i3.columns[1][1] == 1 && i3.columns[1][0] == 0);
	matrix_float2x2 i2 = matrix_identity_float2x2;
	CHECK(i2.columns[0][0] == 1 && i2.columns[0][1] == 0);
}

static void test_quaternion(void)
{
	const float h = 0.70710678f;
	simd_float3 z = {0, 0, 1};
	// a rotation of 90 degrees about z maps x to y
	simd_quatf q = simd_quaternion((float)M_PI_2, z);
	CHECK(near_f(q.vector.z, h) && near_f(q.vector.w, h) && near_f(q.vector.x, 0));
	CHECK(near_f(simd_length(q), 1) && near_f(simd_angle(q), (float)M_PI_2));
	CHECK(near3(simd_axis(q), z));
	CHECK(near3(simd_act(q, (simd_float3){1, 0, 0}), ((simd_float3){0, 1, 0})));
	CHECK(near3(simd_act(simd_conjugate(q), (simd_float3){1, 0, 0}), ((simd_float3){0, -1, 0})));
	simd_quatf q2 = simd_mul(q, q);
	CHECK(near3(simd_act(q2, (simd_float3){1, 0, 0}), ((simd_float3){-1, 0, 0})));
	simd_quatf qi = simd_mul(q, simd_inverse(q));
	CHECK(near_f(qi.vector.w, 1) && near_f(qi.vector.x, 0) && near_f(qi.vector.z, 0));
	// matrix form: columns are the images of the basis vectors
	simd_float3x3 m = simd_matrix3x3(q);
	CHECK(near3(m.columns[0], ((simd_float3){0, 1, 0})) && near3(m.columns[1], ((simd_float3){-1, 0, 0})) && near3(m.columns[2], z));
	simd_quatf back = simd_quaternion(m);
	CHECK(near_f(back.vector.z, h) && near_f(back.vector.w, h));
	simd_float4x4 m4 = simd_matrix4x4(q);
	CHECK(near_f(m4.columns[3].w, 1) && near_f(m4.columns[0].y, 1));
	simd_quatf back4 = simd_quaternion(m4);
	CHECK(near_f(back4.vector.z, h) && near_f(back4.vector.w, h));
	// matrix to quaternion for a half-turn about x exercises the non-positive-trace branch
	simd_quatf qx = simd_quaternion((float)M_PI, (simd_float3){1, 0, 0});
	simd_quatf qxm = simd_quaternion(simd_matrix3x3(qx));
	CHECK(near_f(fabsf(qxm.vector.x), 1) && near_f(qxm.vector.w, 0));
	// shortest rotation between vectors
	simd_quatf f = simd_quaternion((simd_float3){1, 0, 0}, (simd_float3){0, 1, 0});
	CHECK(near3(simd_act(f, (simd_float3){1, 0, 0}), ((simd_float3){0, 1, 0})));
	simd_quatf opp = simd_quaternion((simd_float3){1, 0, 0}, (simd_float3){-1, 0, 0});
	CHECK(near3(simd_act(opp, (simd_float3){1, 0, 0}), ((simd_float3){-1, 0, 0})));
	// interpolation halfway between identity and a quarter turn is an eighth turn
	simd_quatf ident = simd_quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	simd_quatf half = simd_slerp(ident, q, 0.5f);
	CHECK(near_f(simd_angle(half), (float)M_PI_4));
	// exp(log(q)) round-trips a unit quaternion
	simd_quatf el = simd_exp(simd_log(q));
	CHECK(near_f(el.vector.z, h) && near_f(el.vector.w, h));
	// arithmetic and the identity quaternion's axis
	simd_quatf sum = simd_add(q, q);
	CHECK(near_f(sum.vector.w, 2 * h) && near_f(simd_dot(q, q), 1));
	CHECK(simd_equal(simd_sub(sum, q), q));
	// quirks observed on macOS 26 (see the QUIRK comments in quaternion.h)
	simd_quatf zero = simd_quaternion(0.0f, 0.0f, 0.0f, 0.0f);
	simd_quatf zn = simd_normalize(zero); // test_quirk_zero_quaternion_normalizes_to_identity
	CHECK(zn.vector.x == 0 && zn.vector.y == 0 && zn.vector.z == 0 && zn.vector.w == 1);
	simd_float3 ia = simd_axis(ident); // test_quirk_identity_axis_is_nan
	CHECK(isnan(ia.x) && isnan(ia.y) && isnan(ia.z));
	// non-unit axis is used as given
	simd_quatf nu = simd_quaternion((float)M_PI_2, (simd_float3){0, 0, 2});
	CHECK(near_f(nu.vector.z, 2 * h) && near_f(nu.vector.w, h));
	// test_quirk_opposite_vectors_rotation_axis
	const simd_float3 oin[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0.6f, 0.8f, 0}, {0.8f, 0, 0.6f}, {0, 0.6f, -0.8f}, {0.6f, 0.6f, 0.52915f}};
	const simd_float3 oax[] = {{0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0.8f, -0.6f, 0}, {-0.6f, 0, 0.8f}, {0, -0.8f, -0.6f}, {0.707107f, -0.707107f, 0}};
	for (int k = 0; k < 7; k++) {
		simd_quatf oq = simd_quaternion(oin[k], -oin[k]);
		CHECK(near3(oq.vector.xyz, oax[k]) && near_f(oq.vector.w, 0));
	}
	// double precision
	simd_quatd qd = simd_quaternion(M_PI_2, (simd_double3){0, 0, 1});
	simd_double3 r = simd_act(qd, (simd_double3){1, 0, 0});
	CHECK(near_d(r.x, 0) && near_d(r.y, 1));
	simd_double3x3 md = simd_matrix3x3(qd);
	simd_quatd bd = simd_quaternion(md);
	CHECK(near_d(bd.vector.z, 0.7071067811865476) && near_d(bd.vector.w, 0.7071067811865476));
}

int main(void)
{
	test_elementwise();
	test_geometry();
	test_matrix();
	test_quaternion();
	if (g_failures == 0)
		printf("PASS\n");
	return g_failures;
}
