#include "validate.h"

#include "app.h"
#include "render.h"

#include <math.h>
#include <stdio.h>

typedef struct {
    int projected_vertices;
    int projected_edges;
    int slice_segments;
    float min_x;
    float max_x;
    float min_y;
    float max_y;
    float min_z;
    float max_z;
} VisualStats;

static void stats_init(VisualStats *stats)
{
    stats->projected_vertices = 0;
    stats->projected_edges = 0;
    stats->slice_segments = 0;
    stats->min_x = 1000000.0f;
    stats->max_x = -1000000.0f;
    stats->min_y = 1000000.0f;
    stats->max_y = -1000000.0f;
    stats->min_z = 1000000.0f;
    stats->max_z = -1000000.0f;
}

static int stats_add_point(VisualStats *stats, Vec3 p)
{
    if (!finite_float(p.x) || !finite_float(p.y) || !finite_float(p.z)) {
        return 0;
    }

    if (p.x < stats->min_x) {
        stats->min_x = p.x;
    }
    if (p.x > stats->max_x) {
        stats->max_x = p.x;
    }
    if (p.y < stats->min_y) {
        stats->min_y = p.y;
    }
    if (p.y > stats->max_y) {
        stats->max_y = p.y;
    }
    if (p.z < stats->min_z) {
        stats->min_z = p.z;
    }
    if (p.z > stats->max_z) {
        stats->max_z = p.z;
    }

    return 1;
}

static int stats_non_degenerate(const VisualStats *stats)
{
    float dx = stats->max_x - stats->min_x;
    float dy = stats->max_y - stats->min_y;
    float dz = stats->max_z - stats->min_z;

    return finite_float(dx) && finite_float(dy) && finite_float(dz) &&
           dx > 0.01f && dy > 0.01f && dz > 0.01f;
}

static int add_unique_point(Vec3 *points, int count, Vec3 p)
{
    int i;

    for (i = 0; i < count; ++i) {
        float dx = points[i].x - p.x;
        float dy = points[i].y - p.y;
        float dz = points[i].z - p.z;
        if (dx * dx + dy * dy + dz * dz < 0.000001f) {
            return count;
        }
    }

    if (count < MAX_SLICE_POINTS) {
        points[count++] = p;
    }

    return count;
}

static int clip_face_to_w(Vec4 *verts, const Face *face, float slice_w, Vec3 *points)
{
    int count = 0;
    int i;

    for (i = 0; i < face->count; ++i) {
        Vec4 a = verts[face->v[i]];
        Vec4 b = verts[face->v[(i + 1) % face->count]];
        float da = a.w - slice_w;
        float db = b.w - slice_w;

        if (fabsf(da) < GA4D_EPS) {
            count = add_unique_point(points, count, vec4_xyz(a));
        }

        if ((da < 0.0f && db > 0.0f) || (da > 0.0f && db < 0.0f)) {
            float t = da / (da - db);
            Vec4 p = vec4_lerp(a, b, t);
            count = add_unique_point(points, count, vec4_xyz(p));
        }

        if (fabsf(db) < GA4D_EPS) {
            count = add_unique_point(points, count, vec4_xyz(b));
        }
    }

    return count;
}

static int validate_shape_projection(App *app, const Shape *shape, VisualStats *stats)
{
    int i;

    for (i = 0; i < shape->vertex_count; ++i) {
        Vec4 rp = app_rotate_vec4(app, shape->vertices[i]);
        Vec3 p3;

        if (project4_to3(app, rp, &p3)) {
            if (!stats_add_point(stats, p3)) {
                fprintf(stderr, "visual validation: non-finite projected vertex in %s\n",
                        shape->name);
                return 0;
            }
            stats->projected_vertices++;
        }
    }

    for (i = 0; i < shape->edge_count; ++i) {
        Vec4 a = app_rotate_vec4(app, shape->vertices[shape->edges[i].a]);
        Vec4 b = app_rotate_vec4(app, shape->vertices[shape->edges[i].b]);
        Vec3 a3;
        Vec3 b3;

        if (project4_to3(app, a, &a3) && project4_to3(app, b, &b3)) {
            if (!stats_add_point(stats, a3) || !stats_add_point(stats, b3)) {
                fprintf(stderr, "visual validation: non-finite projected edge in %s\n",
                        shape->name);
                return 0;
            }
            stats->projected_edges++;
        }
    }

    return 1;
}

static int validate_shape_slice(App *app, const Shape *shape, VisualStats *stats)
{
    Vec4 rotated[MAX_VERTICES];
    int i;

    for (i = 0; i < shape->vertex_count; ++i) {
        rotated[i] = app_rotate_vec4(app, shape->vertices[i]);
    }

    for (i = 0; i < shape->face_count; ++i) {
        Vec3 points[MAX_SLICE_POINTS];
        int count = clip_face_to_w(rotated, &shape->faces[i], app->slice_w, points);

        if (count < 0 || count > MAX_SLICE_POINTS) {
            fprintf(stderr, "visual validation: invalid slice point count in %s\n",
                    shape->name);
            return 0;
        }
        if (count == 2) {
            if (!stats_add_point(stats, points[0]) || !stats_add_point(stats, points[1])) {
                fprintf(stderr, "visual validation: non-finite slice segment in %s\n",
                        shape->name);
                return 0;
            }
            stats->slice_segments++;
        } else if (count > 2) {
            int j;
            for (j = 0; j < count; ++j) {
                if (!stats_add_point(stats, points[j])) {
                    fprintf(stderr, "visual validation: non-finite slice polygon in %s\n",
                            shape->name);
                    return 0;
                }
                stats->slice_segments++;
            }
        }
    }

    return 1;
}

static int validate_hypersphere(App *app, VisualStats *stats)
{
    const float r4 = 1.65f;
    const int samples = 48;
    int i;

    for (i = 0; i < samples; ++i) {
        float a = 2.0f * GA4D_PI * (float)i / (float)samples;
        float b = GA4D_PI * (float)(i % 17) / 16.0f;
        float c = 2.0f * GA4D_PI * (float)((i * 7) % samples) / (float)samples;
        Vec4 p = vec4(r4 * cosf(a) * sinf(b),
                      r4 * sinf(a) * sinf(b),
                      r4 * cosf(b) * cosf(c),
                      r4 * cosf(b) * sinf(c));
        Vec4 rp = app_rotate_vec4(app, p);
        Vec3 out;

        if (project4_to3(app, rp, &out)) {
            if (!stats_add_point(stats, out)) {
                fprintf(stderr, "visual validation: non-finite hypersphere point\n");
                return 0;
            }
            stats->projected_vertices++;
        }
    }

    if (fabsf(app->slice_w) < r4) {
        float r3 = sqrtf(fmaxf(0.0f, r4 * r4 - app->slice_w * app->slice_w));
        stats->slice_segments += 48;
        if (!stats_add_point(stats, vec3(-r3, 0.0f, 0.0f)) ||
            !stats_add_point(stats, vec3(r3, 0.0f, 0.0f)) ||
            !stats_add_point(stats, vec3(0.0f, -r3, 0.0f)) ||
            !stats_add_point(stats, vec3(0.0f, r3, 0.0f)) ||
            !stats_add_point(stats, vec3(0.0f, 0.0f, -r3)) ||
            !stats_add_point(stats, vec3(0.0f, 0.0f, r3))) {
            fprintf(stderr, "visual validation: non-finite hypersphere slice\n");
            return 0;
        }
    }

    return 1;
}

static int validate_case(App *app, const char *label)
{
    VisualStats stats;
    stats_init(&stats);

    if (app->object == SHAPE_HYPERSPHERE) {
        if (!validate_hypersphere(app, &stats)) {
            return 0;
        }
    } else {
        const Shape *shape = &app->shapes[app->object];
        if (!validate_shape_projection(app, shape, &stats) ||
            !validate_shape_slice(app, shape, &stats)) {
            return 0;
        }
    }

    if (stats.projected_vertices < 4 && stats.projected_edges < 4 && stats.slice_segments < 3) {
        fprintf(stderr, "visual validation: %s produced too little visible geometry\n", label);
        return 0;
    }

    if (!stats_non_degenerate(&stats)) {
        fprintf(stderr, "visual validation: %s produced degenerate bounds\n", label);
        return 0;
    }

    return 1;
}

int run_visual_validation(void)
{
    static const float focals[] = { 2.35f, 3.2f, 4.0f, 8.0f, 12.0f };
    static const float slices[] = { -1.4f, -0.5f, 0.0f, 0.5f, 1.4f };
    App app;
    int preset;
    int i;
    int object;
    int basis;
    int cases = 0;

    app_defaults(&app);

    for (preset = 1; preset <= 8; ++preset) {
        char label[64];
        app_apply_preset(&app, preset);
        snprintf(label, sizeof(label), "preset %d", preset);
        if (!validate_case(&app, label)) {
            return 1;
        }
        cases++;
    }

    for (object = 0; object < SHAPE_COUNT; ++object) {
        for (basis = 0; basis < BASIS_COUNT; ++basis) {
            for (i = 0; i < 5; ++i) {
                char label[96];
                app_defaults(&app);
                app.object = object;
                app.basis_mode = basis;
                app.focal4 = focals[i];
                app.slice_w = slices[i];
                if (basis == BASIS_COORDINATE) {
                    app.coord_angle.e12 = 0.19f + 0.03f * (float)i;
                    app.coord_angle.e13 = -0.24f;
                    app.coord_angle.e14 = 0.52f;
                    app.coord_angle.e23 = 0.13f;
                    app.coord_angle.e24 = -0.21f;
                    app.coord_angle.e34 = 0.37f;
                } else {
                    app.split_angle.l1 = 0.34f;
                    app.split_angle.l2 = -0.18f;
                    app.split_angle.l3 = 0.27f;
                    app.split_angle.r1 = -0.25f;
                    app.split_angle.r2 = 0.16f;
                    app.split_angle.r3 = 0.31f;
                }
                app_update_rotor(&app);
                snprintf(label, sizeof(label), "object %d basis %d sample %d",
                         object, basis, i);
                if (!validate_case(&app, label)) {
                    return 1;
                }
                cases++;
            }
        }
    }

    printf("ga4d visual validation passed (%d cases)\n", cases);
    return 0;
}
