#include "math4d.h"

#include <math.h>
#include <string.h>

static const int plane_masks[PLANE_COUNT] = {
    0x3,  /* e12 */
    0x5,  /* e13 */
    0x9,  /* e14 */
    0x6,  /* e23 */
    0xA,  /* e24 */
    0xC   /* e34 */
};

float clampf(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

int finite_float(float v)
{
    return isfinite((double)v) != 0;
}

Vec3 vec3(float x, float y, float z)
{
    Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

Vec4 vec4(float x, float y, float z, float w)
{
    Vec4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

Vec3 vec4_xyz(Vec4 v)
{
    return vec3(v.x, v.y, v.z);
}

Vec4 vec4_lerp(Vec4 a, Vec4 b, float t)
{
    return vec4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t);
}

float vec4_len(Vec4 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

Color color_rgba(float r, float g, float b, float a)
{
    Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

Color color_mix(Color a, Color b, float t)
{
    t = clampf(t, 0.0f, 1.0f);
    return color_rgba(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t);
}

Color color_for_w(float w, float alpha)
{
    Color cold = color_rgba(0.10f, 0.74f, 1.00f, alpha);
    Color mid = color_rgba(0.92f, 0.94f, 0.94f, alpha);
    Color warm = color_rgba(1.00f, 0.58f, 0.18f, alpha);
    float t = clampf((w + 2.25f) / 4.5f, 0.0f, 1.0f);

    if (t < 0.5f) {
        return color_mix(cold, mid, t * 2.0f);
    }
    return color_mix(mid, warm, (t - 0.5f) * 2.0f);
}

Bivector bivector_zero(void)
{
    Bivector b;
    memset(&b, 0, sizeof(b));
    return b;
}

float bivector_get(const Bivector *b, int plane)
{
    const float *p = &b->e12;
    if (plane < 0 || plane >= PLANE_COUNT) {
        return 0.0f;
    }
    return p[plane];
}

void bivector_set(Bivector *b, int plane, float value)
{
    float *p = &b->e12;
    if (plane >= 0 && plane < PLANE_COUNT) {
        p[plane] = value;
    }
}

Bivector bivector_from_split(SplitBivector s)
{
    Bivector b;

    b.e23 = 0.5f * (s.l1 + s.r1);
    b.e14 = 0.5f * (s.l1 - s.r1);

    b.e13 = -0.5f * (s.l2 + s.r2);
    b.e24 = 0.5f * (s.l2 - s.r2);

    b.e12 = 0.5f * (s.l3 + s.r3);
    b.e34 = 0.5f * (s.l3 - s.r3);

    return b;
}

SplitBivector split_from_bivector(Bivector b)
{
    SplitBivector s;

    s.l1 = b.e23 + b.e14;
    s.r1 = b.e23 - b.e14;

    s.l2 = b.e24 - b.e13;
    s.r2 = -b.e13 - b.e24;

    s.l3 = b.e12 + b.e34;
    s.r3 = b.e12 - b.e34;

    return s;
}

float split_get(const SplitBivector *s, int index)
{
    const float *p = &s->l1;
    if (index < 0 || index >= SPLIT_COUNT) {
        return 0.0f;
    }
    return p[index];
}

void split_set(SplitBivector *s, int index, float value)
{
    float *p = &s->l1;
    if (index >= 0 && index < SPLIT_COUNT) {
        p[index] = value;
    }
}

Multivector mv_zero(void)
{
    Multivector m;
    memset(&m, 0, sizeof(m));
    return m;
}

Multivector mv_basis(int mask, float value)
{
    Multivector m = mv_zero();
    if (mask >= 0 && mask < 16) {
        m.c[mask] = value;
    }
    return m;
}

Multivector mv_add(Multivector a, Multivector b)
{
    int i;
    for (i = 0; i < 16; ++i) {
        a.c[i] += b.c[i];
    }
    return a;
}

Multivector mv_scale(Multivector a, float scale)
{
    int i;
    for (i = 0; i < 16; ++i) {
        a.c[i] *= scale;
    }
    return a;
}

static int bit_count(int x)
{
    int n = 0;
    while (x) {
        n += x & 1;
        x >>= 1;
    }
    return n;
}

static int gp_sign(int a, int b)
{
    int sign = 1;
    int i;

    for (i = 0; i < 4; ++i) {
        if (a & (1 << i)) {
            int lower = b & ((1 << i) - 1);
            if (bit_count(lower) & 1) {
                sign = -sign;
            }
        }
    }

    return sign;
}

Multivector mv_gp(Multivector a, Multivector b)
{
    Multivector out = mv_zero();
    int i;
    int j;

    for (i = 0; i < 16; ++i) {
        if (a.c[i] == 0.0f) {
            continue;
        }
        for (j = 0; j < 16; ++j) {
            int mask;
            int sign;
            if (b.c[j] == 0.0f) {
                continue;
            }
            mask = i ^ j;
            sign = gp_sign(i, j);
            out.c[mask] += (float)sign * a.c[i] * b.c[j];
        }
    }

    return out;
}

Multivector mv_reverse(Multivector a)
{
    int i;
    for (i = 0; i < 16; ++i) {
        int grade = bit_count(i);
        int flips = grade * (grade - 1) / 2;
        if (flips & 1) {
            a.c[i] = -a.c[i];
        }
    }
    return a;
}

Multivector mv_normalize_rotor(Multivector r)
{
    Multivector n = mv_gp(r, mv_reverse(r));
    float scalar = n.c[0];

    if (scalar > GA4D_EPS && finite_float(scalar)) {
        return mv_scale(r, 1.0f / sqrtf(scalar));
    }
    return mv_basis(0, 1.0f);
}

Multivector rotor_from_plane(int plane, float angle)
{
    Multivector r = mv_zero();
    int mask;

    if (plane < 0 || plane >= PLANE_COUNT) {
        return mv_basis(0, 1.0f);
    }

    mask = plane_masks[plane];
    r.c[0] = cosf(angle * 0.5f);
    r.c[mask] = -sinf(angle * 0.5f);
    return mv_normalize_rotor(r);
}

Multivector rotor_from_bivector_sequence(Bivector b)
{
    Multivector total = mv_basis(0, 1.0f);
    int i;

    for (i = 0; i < PLANE_COUNT; ++i) {
        float angle = bivector_get(&b, i);
        if (fabsf(angle) > GA4D_EPS) {
            Multivector rp = rotor_from_plane(i, angle);
            total = mv_gp(rp, total);
        }
    }

    return mv_normalize_rotor(total);
}

Vec4 rotor_apply_vec4(Multivector rotor, Vec4 v)
{
    Multivector mv = mv_zero();
    Multivector out;

    mv.c[0x1] = v.x;
    mv.c[0x2] = v.y;
    mv.c[0x4] = v.z;
    mv.c[0x8] = v.w;

    out = mv_gp(mv_gp(rotor, mv), mv_reverse(rotor));
    return vec4(out.c[0x1], out.c[0x2], out.c[0x4], out.c[0x8]);
}
