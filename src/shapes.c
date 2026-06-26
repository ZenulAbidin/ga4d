#include "shapes.h"

#include "math4d.h"

#include <string.h>

static void shape_clear(Shape *shape, const char *name)
{
    memset(shape, 0, sizeof(*shape));
    shape->name = name;
}

static void shape_add_edge(Shape *shape, int a, int b)
{
    if (shape->edge_count >= MAX_EDGES) {
        return;
    }
    shape->edges[shape->edge_count].a = a;
    shape->edges[shape->edge_count].b = b;
    shape->edge_count++;
}

static void shape_add_face3(Shape *shape, int a, int b, int c)
{
    Face *face;

    if (shape->face_count >= MAX_FACES) {
        return;
    }

    face = &shape->faces[shape->face_count++];
    face->count = 3;
    face->v[0] = a;
    face->v[1] = b;
    face->v[2] = c;
}

static void shape_add_face4(Shape *shape, int a, int b, int c, int d)
{
    Face *face;

    if (shape->face_count >= MAX_FACES) {
        return;
    }

    face = &shape->faces[shape->face_count++];
    face->count = 4;
    face->v[0] = a;
    face->v[1] = b;
    face->v[2] = c;
    face->v[3] = d;
}

static int cross_index(int axis, int sign)
{
    return axis * 2 + (sign > 0 ? 1 : 0);
}

static void build_tesseract(Shape *shape)
{
    int bits;
    int dim;
    int dim_a;
    int dim_b;
    int fixed_a;
    int fixed_b;

    shape_clear(shape, "TESSERACT");

    for (bits = 0; bits < 16; ++bits) {
        float x = (bits & 1) ? 1.0f : -1.0f;
        float y = (bits & 2) ? 1.0f : -1.0f;
        float z = (bits & 4) ? 1.0f : -1.0f;
        float w = (bits & 8) ? 1.0f : -1.0f;
        shape->vertices[shape->vertex_count++] = vec4(x, y, z, w);
    }

    for (bits = 0; bits < 16; ++bits) {
        for (dim = 0; dim < 4; ++dim) {
            int other = bits ^ (1 << dim);
            if (bits < other) {
                shape_add_edge(shape, bits, other);
            }
        }
    }

    for (dim_a = 0; dim_a < 4; ++dim_a) {
        for (dim_b = dim_a + 1; dim_b < 4; ++dim_b) {
            int other_dims[2];
            int n = 0;

            for (dim = 0; dim < 4; ++dim) {
                if (dim != dim_a && dim != dim_b) {
                    other_dims[n++] = dim;
                }
            }

            for (fixed_a = 0; fixed_a < 2; ++fixed_a) {
                for (fixed_b = 0; fixed_b < 2; ++fixed_b) {
                    int base = 0;
                    int v0;
                    int v1;
                    int v2;
                    int v3;

                    if (fixed_a) {
                        base |= 1 << other_dims[0];
                    }
                    if (fixed_b) {
                        base |= 1 << other_dims[1];
                    }

                    v0 = base;
                    v1 = base | (1 << dim_a);
                    v2 = base | (1 << dim_a) | (1 << dim_b);
                    v3 = base | (1 << dim_b);
                    shape_add_face4(shape, v0, v1, v2, v3);
                }
            }
        }
    }
}

static void build_16cell(Shape *shape)
{
    int axis;
    int i;
    int j;
    int a;
    int b;
    int c;
    int sx;
    int sy;
    int sz;
    const float scale = 1.65f;

    shape_clear(shape, "16-CELL");

    for (axis = 0; axis < 4; ++axis) {
        Vec4 neg = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        Vec4 pos = vec4(0.0f, 0.0f, 0.0f, 0.0f);

        if (axis == 0) {
            neg.x = -scale;
            pos.x = scale;
        } else if (axis == 1) {
            neg.y = -scale;
            pos.y = scale;
        } else if (axis == 2) {
            neg.z = -scale;
            pos.z = scale;
        } else {
            neg.w = -scale;
            pos.w = scale;
        }

        shape->vertices[shape->vertex_count++] = neg;
        shape->vertices[shape->vertex_count++] = pos;
    }

    for (i = 0; i < shape->vertex_count; ++i) {
        for (j = i + 1; j < shape->vertex_count; ++j) {
            if (i / 2 != j / 2) {
                shape_add_edge(shape, i, j);
            }
        }
    }

    for (a = 0; a < 4; ++a) {
        for (b = a + 1; b < 4; ++b) {
            for (c = b + 1; c < 4; ++c) {
                for (sx = -1; sx <= 1; sx += 2) {
                    for (sy = -1; sy <= 1; sy += 2) {
                        for (sz = -1; sz <= 1; sz += 2) {
                            shape_add_face3(shape,
                                            cross_index(a, sx),
                                            cross_index(b, sy),
                                            cross_index(c, sz));
                        }
                    }
                }
            }
        }
    }
}

static void build_simplex5(Shape *shape)
{
    static const float basis[5][4] = {
        {  0.7071067812f,  0.4082482905f,  0.2886751346f,  0.2236067977f },
        { -0.7071067812f,  0.4082482905f,  0.2886751346f,  0.2236067977f },
        {  0.0f,          -0.8164965809f,  0.2886751346f,  0.2236067977f },
        {  0.0f,           0.0f,          -0.8660254038f,  0.2236067977f },
        {  0.0f,           0.0f,           0.0f,          -0.8944271909f }
    };
    const float scale = 1.95f;
    int i;
    int j;
    int k;

    shape_clear(shape, "5-CELL");

    for (i = 0; i < 5; ++i) {
        shape->vertices[shape->vertex_count++] = vec4(
            basis[i][0] * scale,
            basis[i][1] * scale,
            basis[i][2] * scale,
            basis[i][3] * scale);
    }

    for (i = 0; i < 5; ++i) {
        for (j = i + 1; j < 5; ++j) {
            shape_add_edge(shape, i, j);
        }
    }

    for (i = 0; i < 5; ++i) {
        for (j = i + 1; j < 5; ++j) {
            for (k = j + 1; k < 5; ++k) {
                shape_add_face3(shape, i, j, k);
            }
        }
    }
}

void build_shapes(Shape shapes[3])
{
    build_tesseract(&shapes[SHAPE_TESSERACT]);
    build_16cell(&shapes[SHAPE_CELL16]);
    build_simplex5(&shapes[SHAPE_SIMPLEX5]);
}
