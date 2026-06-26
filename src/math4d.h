#ifndef GA4D_MATH4D_H
#define GA4D_MATH4D_H

#include "common.h"

typedef struct {
    float e12;
    float e13;
    float e14;
    float e23;
    float e24;
    float e34;
} Bivector;

typedef struct {
    float l1;
    float l2;
    float l3;
    float r1;
    float r2;
    float r3;
} SplitBivector;

typedef struct {
    float c[16];
} Multivector;

float clampf(float v, float lo, float hi);
int finite_float(float v);

Vec3 vec3(float x, float y, float z);
Vec4 vec4(float x, float y, float z, float w);
Vec3 vec4_xyz(Vec4 v);
Vec4 vec4_lerp(Vec4 a, Vec4 b, float t);
float vec4_len(Vec4 v);

Color color_rgba(float r, float g, float b, float a);
Color color_mix(Color a, Color b, float t);
Color color_for_w(float w, float alpha);

Bivector bivector_zero(void);
float bivector_get(const Bivector *b, int plane);
void bivector_set(Bivector *b, int plane, float value);
Bivector bivector_from_split(SplitBivector s);
SplitBivector split_from_bivector(Bivector b);
float split_get(const SplitBivector *s, int index);
void split_set(SplitBivector *s, int index, float value);

Multivector mv_zero(void);
Multivector mv_basis(int mask, float value);
Multivector mv_add(Multivector a, Multivector b);
Multivector mv_scale(Multivector a, float scale);
Multivector mv_gp(Multivector a, Multivector b);
Multivector mv_reverse(Multivector a);
Multivector mv_normalize_rotor(Multivector r);

Multivector rotor_from_plane(int plane, float angle);
Multivector rotor_from_bivector_sequence(Bivector b);
Vec4 rotor_apply_vec4(Multivector rotor, Vec4 v);

#endif
