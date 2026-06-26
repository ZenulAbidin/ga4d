#include "extrude.h"

#include "math4d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#define ICON_MAX_GRID 32

typedef struct {
    float x;
    float y;
    float z;
} Vertex3;

typedef struct {
    int a;
    int b;
} Edge3;

typedef struct {
    int count;
    int v[4];
} Face3;

static void shape_clear(Shape *shape, const char *name)
{
    memset(shape, 0, sizeof(*shape));
    shape->name = name;
}

static int shape_add_vertex(Shape *shape, Vec4 p)
{
    if (shape->vertex_count >= MAX_VERTICES) {
        return -1;
    }

    shape->vertices[shape->vertex_count] = p;
    return shape->vertex_count++;
}

static void shape_add_edge_unique(Shape *shape, int a, int b)
{
    int i;

    if (a < 0 || b < 0 || a == b) {
        return;
    }
    if (a > b) {
        int tmp = a;
        a = b;
        b = tmp;
    }

    for (i = 0; i < shape->edge_count; ++i) {
        if (shape->edges[i].a == a && shape->edges[i].b == b) {
            return;
        }
    }

    if (shape->edge_count >= MAX_EDGES) {
        return;
    }

    shape->edges[shape->edge_count].a = a;
    shape->edges[shape->edge_count].b = b;
    shape->edge_count++;
}

static void shape_add_face(Shape *shape, int count, int a, int b, int c, int d)
{
    Face *face;

    if (count < 3 || count > 4 || shape->face_count >= MAX_FACES) {
        return;
    }
    if (a < 0 || b < 0 || c < 0 || (count == 4 && d < 0)) {
        return;
    }

    face = &shape->faces[shape->face_count++];
    face->count = count;
    face->v[0] = a;
    face->v[1] = b;
    face->v[2] = c;
    face->v[3] = d;

    shape_add_edge_unique(shape, a, b);
    shape_add_edge_unique(shape, b, c);
    if (count == 3) {
        shape_add_edge_unique(shape, c, a);
    } else {
        shape_add_edge_unique(shape, c, d);
        shape_add_edge_unique(shape, d, a);
    }
}

static int map_index(int width, int height, int x, int y, int layer)
{
    int stride = (width + 1) * (height + 1);
    return layer * stride + y * (width + 1) + x;
}

static int grid_vertex(Shape *shape, int *map, int width, int height,
                       int x, int y, int layer, float scale, float depth)
{
    int index = map_index(width, height, x, y, layer);
    float max_dim = (width > height) ? (float)width : (float)height;
    float step = scale / max_dim;
    Vec4 p;

    if (map[index] >= 0) {
        return map[index];
    }

    p.x = ((float)x - (float)width * 0.5f) * step;
    p.y = ((float)height * 0.5f - (float)y) * step;
    p.z = 0.0f;
    p.w = layer ? depth : -depth;

    map[index] = shape_add_vertex(shape, p);
    return map[index];
}

static int filled_at(const unsigned char *filled, int width, int height, int x, int y)
{
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return 0;
    }
    return filled[y * width + x] != 0;
}

static void add_boundary_face(Shape *shape, int *map, int width, int height,
                              int x0, int y0, int x1, int y1,
                              float scale, float depth)
{
    int a0 = grid_vertex(shape, map, width, height, x0, y0, 0, scale, depth);
    int b0 = grid_vertex(shape, map, width, height, x1, y1, 0, scale, depth);
    int b1 = grid_vertex(shape, map, width, height, x1, y1, 1, scale, depth);
    int a1 = grid_vertex(shape, map, width, height, x0, y0, 1, scale, depth);

    shape_add_face(shape, 4, a0, b0, b1, a1);
}

static int build_grid_extrusion(Shape *shape, const char *name,
                                const unsigned char *filled,
                                int width, int height,
                                float scale, float depth)
{
    int map[(ICON_MAX_GRID + 1) * (ICON_MAX_GRID + 1) * 2];
    int x;
    int y;
    int i;

    if (width <= 0 || height <= 0 ||
        width > ICON_MAX_GRID || height > ICON_MAX_GRID) {
        return 0;
    }

    shape_clear(shape, name);
    for (i = 0; i < (ICON_MAX_GRID + 1) * (ICON_MAX_GRID + 1) * 2; ++i) {
        map[i] = -1;
    }

    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            if (!filled_at(filled, width, height, x, y)) {
                continue;
            }
            if (!filled_at(filled, width, height, x, y - 1)) {
                add_boundary_face(shape, map, width, height, x, y, x + 1, y, scale, depth);
            }
            if (!filled_at(filled, width, height, x + 1, y)) {
                add_boundary_face(shape, map, width, height, x + 1, y, x + 1, y + 1, scale, depth);
            }
            if (!filled_at(filled, width, height, x, y + 1)) {
                add_boundary_face(shape, map, width, height, x + 1, y + 1, x, y + 1, scale, depth);
            }
            if (!filled_at(filled, width, height, x - 1, y)) {
                add_boundary_face(shape, map, width, height, x, y + 1, x, y, scale, depth);
            }
        }
    }

    return shape->vertex_count > 0 && shape->edge_count > 0 && shape->face_count > 0;
}

static int build_bitmap_extrusion(Shape *shape, const char *name,
                                  const char **rows, int height,
                                  float scale, float depth)
{
    unsigned char filled[ICON_MAX_GRID * ICON_MAX_GRID];
    int width;
    int x;
    int y;

    if (height <= 0 || height > ICON_MAX_GRID || !rows || !rows[0]) {
        return 0;
    }

    width = (int)strlen(rows[0]);
    if (width <= 0 || width > ICON_MAX_GRID) {
        return 0;
    }

    memset(filled, 0, sizeof(filled));
    for (y = 0; y < height; ++y) {
        if ((int)strlen(rows[y]) != width) {
            return 0;
        }
        for (x = 0; x < width; ++x) {
            filled[y * width + x] = rows[y][x] == '.' ? 0 : 1;
        }
    }

    return build_grid_extrusion(shape, name, filled, width, height, scale, depth);
}

void build_icon_arrow_shape(Shape *shape)
{
    static const char *rows[] = {
        ".......##.......",
        "......####......",
        ".....######.....",
        "....########....",
        "...##########...",
        "..############..",
        ".##############.",
        "################",
        "......####......",
        "......####......",
        "......####......",
        "......####......",
        "......####......",
        "......####......",
        "......####......",
        "......####......"
    };

    build_bitmap_extrusion(shape, "ICON ARROW-W", rows, 16, 3.1f, 1.05f);
}

void build_icon_star_shape(Shape *shape)
{
    static const char *rows[] = {
        ".......##.......",
        ".......##.......",
        "......####......",
        "......####......",
        "..############..",
        "...##########...",
        "....########....",
        ".....######.....",
        "....########....",
        "...####..####...",
        "..###......###..",
        ".##..........##.",
        ".....######.....",
        "....##....##....",
        "...##......##...",
        "..##........##.."
    };

    build_bitmap_extrusion(shape, "ICON STAR-W", rows, 16, 3.1f, 1.05f);
}

void build_png_sample_shape(Shape *shape)
{
    static const char *rows[] = {
        ".....######.....",
        "...##########...",
        "..####....####..",
        ".###........###.",
        ".###........###.",
        "###..........###",
        "###..........###",
        "###..........###",
        "###..........###",
        "###..........###",
        "###..........###",
        ".###........###.",
        ".###........###.",
        "..####....####..",
        "...##########...",
        ".....######....."
    };

    build_bitmap_extrusion(shape, "PNG SAMPLE-W", rows, 16, 3.1f, 1.05f);
}

static int source_has_transparency(const unsigned char *rgba, int pixels)
{
    int i;

    for (i = 0; i < pixels; ++i) {
        if (rgba[i * 4 + 3] < 250) {
            return 1;
        }
    }

    return 0;
}

int build_png_icon_shape(Shape *shape, const char *path, char *status, size_t status_size)
{
    unsigned char filled[ICON_MAX_GRID * ICON_MAX_GRID];
    unsigned char *rgba;
    int image_w;
    int image_h;
    int comp;
    int grid_w;
    int grid_h;
    int has_alpha;
    int gx;
    int gy;
    int filled_count = 0;

    if (!path || path[0] == '\0') {
        snprintf(status, status_size, "PNG path is empty");
        return 0;
    }

    if (!stbi_info(path, &image_w, &image_h, &comp)) {
        snprintf(status, status_size, "not a readable PNG: %s", stbi_failure_reason());
        return 0;
    }
    if (image_w <= 0 || image_h <= 0 || image_w > 4096 || image_h > 4096) {
        snprintf(status, status_size, "PNG dimensions are unsupported: %dx%d", image_w, image_h);
        return 0;
    }

    rgba = stbi_load(path, &image_w, &image_h, &comp, 4);
    if (!rgba) {
        snprintf(status, status_size, "PNG decode failed: %s", stbi_failure_reason());
        return 0;
    }

    if (image_w >= image_h) {
        grid_w = ICON_MAX_GRID;
        grid_h = (image_h * ICON_MAX_GRID + image_w / 2) / image_w;
        if (grid_h < 1) {
            grid_h = 1;
        }
    } else {
        grid_h = ICON_MAX_GRID;
        grid_w = (image_w * ICON_MAX_GRID + image_h / 2) / image_h;
        if (grid_w < 1) {
            grid_w = 1;
        }
    }

    memset(filled, 0, sizeof(filled));
    has_alpha = source_has_transparency(rgba, image_w * image_h);

    for (gy = 0; gy < grid_h; ++gy) {
        for (gx = 0; gx < grid_w; ++gx) {
            int x0 = gx * image_w / grid_w;
            int x1 = (gx + 1) * image_w / grid_w;
            int y0 = gy * image_h / grid_h;
            int y1 = (gy + 1) * image_h / grid_h;
            int x;
            int y;
            int samples = 0;
            int alpha_sum = 0;
            int luma_sum = 0;

            if (x1 <= x0) {
                x1 = x0 + 1;
            }
            if (y1 <= y0) {
                y1 = y0 + 1;
            }

            for (y = y0; y < y1 && y < image_h; ++y) {
                for (x = x0; x < x1 && x < image_w; ++x) {
                    unsigned char *p = rgba + (y * image_w + x) * 4;
                    int luma = ((int)p[0] * 77 + (int)p[1] * 150 + (int)p[2] * 29) >> 8;
                    alpha_sum += p[3];
                    luma_sum += luma;
                    samples++;
                }
            }

            if (samples > 0) {
                int avg_alpha = alpha_sum / samples;
                int avg_luma = luma_sum / samples;
                int on = has_alpha ? (avg_alpha > 32) : (avg_luma < 245);
                filled[gy * grid_w + gx] = on ? 1 : 0;
                if (on) {
                    filled_count++;
                }
            }
        }
    }

    stbi_image_free(rgba);

    if (filled_count <= 0) {
        snprintf(status, status_size, "PNG had no opaque or dark icon pixels");
        return 0;
    }

    if (!build_grid_extrusion(shape, "PNG ICON-W", filled, grid_w, grid_h, 3.1f, 1.05f)) {
        snprintf(status, status_size, "PNG icon mesh exceeded internal limits");
        return 0;
    }

    snprintf(status, status_size, "loaded %dx%d PNG as %dx%d W-extrusion",
             image_w, image_h, grid_w, grid_h);
    return 1;
}

static void build_wire3_extrusion(Shape *shape, const char *name,
                                  const Vertex3 *vertices, int vertex_count,
                                  const Edge3 *edges, int edge_count,
                                  const Face3 *faces, int face_count,
                                  float scale, float depth)
{
    int lower[64];
    int upper[64];
    int i;

    shape_clear(shape, name);
    if (vertex_count > 64) {
        return;
    }

    for (i = 0; i < vertex_count; ++i) {
        lower[i] = shape_add_vertex(shape, vec4(vertices[i].x * scale,
                                                vertices[i].y * scale,
                                                vertices[i].z * scale,
                                                -depth));
        upper[i] = shape_add_vertex(shape, vec4(vertices[i].x * scale,
                                                vertices[i].y * scale,
                                                vertices[i].z * scale,
                                                depth));
    }

    for (i = 0; i < edge_count; ++i) {
        int a0 = lower[edges[i].a];
        int b0 = lower[edges[i].b];
        int a1 = upper[edges[i].a];
        int b1 = upper[edges[i].b];
        shape_add_face(shape, 4, a0, b0, b1, a1);
    }

    for (i = 0; i < face_count; ++i) {
        if (faces[i].count == 3) {
            shape_add_face(shape, 3,
                           lower[faces[i].v[0]],
                           lower[faces[i].v[1]],
                           lower[faces[i].v[2]],
                           -1);
            shape_add_face(shape, 3,
                           upper[faces[i].v[0]],
                           upper[faces[i].v[2]],
                           upper[faces[i].v[1]],
                           -1);
        } else if (faces[i].count == 4) {
            shape_add_face(shape, 4,
                           lower[faces[i].v[0]],
                           lower[faces[i].v[1]],
                           lower[faces[i].v[2]],
                           lower[faces[i].v[3]]);
            shape_add_face(shape, 4,
                           upper[faces[i].v[0]],
                           upper[faces[i].v[3]],
                           upper[faces[i].v[2]],
                           upper[faces[i].v[1]]);
        }
    }
}

void build_house_w_shape(Shape *shape)
{
    static const Vertex3 vertices[] = {
        { -1.0f, -0.8f, -0.55f },
        {  1.0f, -0.8f, -0.55f },
        {  1.0f,  0.25f, -0.55f },
        { -1.0f,  0.25f, -0.55f },
        { -1.0f, -0.8f,  0.55f },
        {  1.0f, -0.8f,  0.55f },
        {  1.0f,  0.25f,  0.55f },
        { -1.0f,  0.25f,  0.55f },
        {  0.0f,  0.95f, -0.55f },
        {  0.0f,  0.95f,  0.55f }
    };
    static const Edge3 edges[] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
        { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
        { 3, 8 }, { 2, 8 }, { 7, 9 }, { 6, 9 }, { 8, 9 }
    };
    static const Face3 faces[] = {
        { 4, { 0, 1, 2, 3 } },
        { 4, { 4, 7, 6, 5 } },
        { 4, { 0, 4, 5, 1 } },
        { 4, { 1, 5, 6, 2 } },
        { 4, { 0, 3, 7, 4 } },
        { 4, { 3, 8, 9, 7 } },
        { 4, { 2, 6, 9, 8 } },
        { 3, { 3, 2, 8, 0 } },
        { 3, { 7, 9, 6, 0 } }
    };

    build_wire3_extrusion(shape, "3D HOUSE-W",
                          vertices, (int)(sizeof(vertices) / sizeof(vertices[0])),
                          edges, (int)(sizeof(edges) / sizeof(edges[0])),
                          faces, (int)(sizeof(faces) / sizeof(faces[0])),
                          1.25f, 1.05f);
}
