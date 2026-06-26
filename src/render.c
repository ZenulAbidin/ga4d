#include "render.h"

#include "text.h"

#include <math.h>
#include <stdio.h>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

static void gl_color(Color c)
{
    glColor4f(c.r, c.g, c.b, c.a);
}

int project4_to3(const App *app, Vec4 p, Vec3 *out)
{
    float denom = app->focal4 - p.w;
    float scale;

    if (denom < 0.14f) {
        return 0;
    }

    scale = app->focal4 / denom;
    if (!finite_float(scale) || fabsf(scale) > 18.0f) {
        return 0;
    }

    out->x = p.x * scale;
    out->y = p.y * scale;
    out->z = p.z * scale;
    return finite_float(out->x) && finite_float(out->y) && finite_float(out->z);
}

static void draw_line3(Vec3 a, Vec3 b, Color c)
{
    gl_color(c);
    glVertex3f(a.x, a.y, a.z);
    glVertex3f(b.x, b.y, b.z);
}

static void draw_projected_segment(const App *app, Vec4 a4, Vec4 b4, float alpha)
{
    Vec4 ra = app_rotate_vec4(app, a4);
    Vec4 rb = app_rotate_vec4(app, b4);
    Vec3 a3;
    Vec3 b3;
    Color c;

    if (!project4_to3(app, ra, &a3) || !project4_to3(app, rb, &b3)) {
        return;
    }

    if (app->draw_depth_cue) {
        c = color_for_w((ra.w + rb.w) * 0.5f, alpha);
    } else {
        c = color_rgba(0.85f, 0.88f, 0.92f, alpha);
    }
    draw_line3(a3, b3, c);
}

static void draw_projected_point(const App *app, Vec4 p4, float size, Color c)
{
    Vec4 rp = app_rotate_vec4(app, p4);
    Vec3 p3;

    if (!project4_to3(app, rp, &p3)) {
        return;
    }

    glPointSize(size);
    glBegin(GL_POINTS);
    gl_color(c);
    glVertex3f(p3.x, p3.y, p3.z);
    glEnd();
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

static void draw_shape_projection(const App *app, const Shape *shape, float alpha)
{
    int i;

    glBegin(GL_LINES);
    for (i = 0; i < shape->edge_count; ++i) {
        Vec4 a = shape->vertices[shape->edges[i].a];
        Vec4 b = shape->vertices[shape->edges[i].b];
        draw_projected_segment(app, a, b, alpha);
    }
    glEnd();

    if (app->show_points) {
        for (i = 0; i < shape->vertex_count; ++i) {
            draw_projected_point(app, shape->vertices[i], 4.0f,
                                 color_rgba(1.0f, 1.0f, 1.0f, alpha * 0.9f));
        }
    }
}

static void draw_shape_slice(const App *app, const Shape *shape, float alpha)
{
    Vec4 rotated[MAX_VERTICES];
    int i;
    Color c = color_rgba(0.95f, 0.96f, 0.96f, alpha);

    for (i = 0; i < shape->vertex_count; ++i) {
        rotated[i] = app_rotate_vec4(app, shape->vertices[i]);
    }

    glBegin(GL_LINES);
    for (i = 0; i < shape->face_count; ++i) {
        Vec3 points[MAX_SLICE_POINTS];
        int count = clip_face_to_w(rotated, &shape->faces[i], app->slice_w, points);

        if (count == 2) {
            draw_line3(points[0], points[1], c);
        } else if (count > 2) {
            int j;
            for (j = 0; j < count; ++j) {
                draw_line3(points[j], points[(j + 1) % count], c);
            }
        }
    }
    glEnd();
}

static void draw_sphere_wire3(float radius, int rings, int segments, Color color)
{
    int ring;
    int seg;

    if (radius < 0.001f) {
        return;
    }

    glBegin(GL_LINES);
    for (ring = -rings + 1; ring < rings; ++ring) {
        float z = radius * (float)ring / (float)rings;
        float rr = sqrtf(fmaxf(0.0f, radius * radius - z * z));
        for (seg = 0; seg < segments; ++seg) {
            float a0 = 2.0f * GA4D_PI * (float)seg / (float)segments;
            float a1 = 2.0f * GA4D_PI * (float)(seg + 1) / (float)segments;
            draw_line3(vec3(rr * cosf(a0), rr * sinf(a0), z),
                       vec3(rr * cosf(a1), rr * sinf(a1), z),
                       color);
        }
    }

    for (ring = 0; ring < rings * 2; ++ring) {
        float phi = GA4D_PI * (float)ring / (float)rings;
        for (seg = 0; seg < segments; ++seg) {
            float a0 = 2.0f * GA4D_PI * (float)seg / (float)segments;
            float a1 = 2.0f * GA4D_PI * (float)(seg + 1) / (float)segments;
            draw_line3(vec3(radius * sinf(a0) * cosf(phi),
                            radius * sinf(a0) * sinf(phi),
                            radius * cosf(a0)),
                       vec3(radius * sinf(a1) * cosf(phi),
                            radius * sinf(a1) * sinf(phi),
                            radius * cosf(a1)),
                       color);
        }
    }
    glEnd();
}

static void draw_hypersphere_projection(const App *app, float alpha)
{
    const float r4 = 1.65f;
    const int w_layers = 7;
    const int rings = 4;
    const int segments = 48;
    int layer;
    int ring;
    int seg;

    glBegin(GL_LINES);
    for (layer = -w_layers; layer <= w_layers; ++layer) {
        float w = r4 * (float)layer / (float)w_layers;
        float r3 = sqrtf(fmaxf(0.0f, r4 * r4 - w * w));
        Color c = color_for_w(w, alpha * (0.45f + 0.55f * (r3 / r4)));

        for (ring = -rings + 1; ring < rings; ++ring) {
            float z = r3 * (float)ring / (float)rings;
            float rr = sqrtf(fmaxf(0.0f, r3 * r3 - z * z));
            for (seg = 0; seg < segments; ++seg) {
                float a0 = 2.0f * GA4D_PI * (float)seg / (float)segments;
                float a1 = 2.0f * GA4D_PI * (float)(seg + 1) / (float)segments;
                Vec4 p0 = vec4(rr * cosf(a0), rr * sinf(a0), z, w);
                Vec4 p1 = vec4(rr * cosf(a1), rr * sinf(a1), z, w);
                Vec4 rp0 = app_rotate_vec4(app, p0);
                Vec4 rp1 = app_rotate_vec4(app, p1);
                Vec3 a3;
                Vec3 b3;
                if (project4_to3(app, rp0, &a3) && project4_to3(app, rp1, &b3)) {
                    draw_line3(a3, b3, c);
                }
            }
        }

        for (ring = 0; ring < rings * 2; ++ring) {
            float phi = GA4D_PI * (float)ring / (float)rings;
            for (seg = 0; seg < segments; ++seg) {
                float a0 = 2.0f * GA4D_PI * (float)seg / (float)segments;
                float a1 = 2.0f * GA4D_PI * (float)(seg + 1) / (float)segments;
                Vec4 p0 = vec4(r3 * sinf(a0) * cosf(phi),
                               r3 * sinf(a0) * sinf(phi),
                               r3 * cosf(a0),
                               w);
                Vec4 p1 = vec4(r3 * sinf(a1) * cosf(phi),
                               r3 * sinf(a1) * sinf(phi),
                               r3 * cosf(a1),
                               w);
                Vec4 rp0 = app_rotate_vec4(app, p0);
                Vec4 rp1 = app_rotate_vec4(app, p1);
                Vec3 a3;
                Vec3 b3;
                if (project4_to3(app, rp0, &a3) && project4_to3(app, rp1, &b3)) {
                    draw_line3(a3, b3, c);
                }
            }
        }
    }
    glEnd();
}

static void draw_hypersphere_slice(const App *app, float alpha)
{
    const float r4 = 1.65f;
    float w = clampf(app->slice_w, -r4, r4);
    float r3 = sqrtf(fmaxf(0.0f, r4 * r4 - w * w));
    draw_sphere_wire3(r3, 5, 56, color_rgba(0.95f, 0.96f, 0.96f, alpha));
}

static void draw_grid(void)
{
    int i;
    Color minor = color_rgba(0.22f, 0.25f, 0.28f, 0.45f);
    Color major = color_rgba(0.35f, 0.38f, 0.42f, 0.60f);

    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (i = -6; i <= 6; ++i) {
        Color c = (i == 0) ? major : minor;
        draw_line3(vec3((float)i, -6.0f, 0.0f), vec3((float)i, 6.0f, 0.0f), c);
        draw_line3(vec3(-6.0f, (float)i, 0.0f), vec3(6.0f, (float)i, 0.0f), c);
    }
    glEnd();
}

static void draw_axes(const App *app)
{
    Vec4 axes[4][2] = {
        { { -2.2f, 0.0f, 0.0f, 0.0f }, { 2.2f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, -2.2f, 0.0f, 0.0f }, { 0.0f, 2.2f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, -2.2f, 0.0f }, { 0.0f, 0.0f, 2.2f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, -2.2f }, { 0.0f, 0.0f, 0.0f, 2.2f } }
    };
    Color colors[4] = {
        { 1.0f, 0.25f, 0.22f, 1.0f },
        { 0.30f, 0.92f, 0.36f, 1.0f },
        { 0.30f, 0.55f, 1.0f, 1.0f },
        { 1.0f, 0.25f, 0.95f, 1.0f }
    };
    int i;

    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (i = 0; i < 4; ++i) {
        Vec4 a = app_rotate_vec4(app, axes[i][0]);
        Vec4 b = app_rotate_vec4(app, axes[i][1]);
        Vec3 a3;
        Vec3 b3;

        if (project4_to3(app, a, &a3) && project4_to3(app, b, &b3)) {
            draw_line3(a3, b3, colors[i]);
        }
    }
    glEnd();
}

static void perspective_gl(float fovy_degrees, float aspect, float znear, float zfar)
{
    float ymax = znear * tanf(fovy_degrees * GA4D_PI / 360.0f);
    float xmax = ymax * aspect;
    glFrustum(-xmax, xmax, -ymax, ymax, znear, zfar);
}

static void begin_world(const App *app)
{
    float aspect = (app->height > 0) ? (float)app->width / (float)app->height : 1.0f;

    glViewport(0, 0, app->width, app->height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    perspective_gl(50.0f, aspect, 0.05f, 80.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(app->pan_x, app->pan_y, -app->camera_distance);
    glRotatef(app->camera_pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(app->camera_yaw, 0.0f, 1.0f, 0.0f);
}

static void begin_overlay(const App *app)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (double)app->width, (double)app->height, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
}

static void end_overlay(void)
{
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void draw_overlay(App *app)
{
    char line[256];
    float y = 14.0f;
    float scale = 2.0f;
    Color text = color_rgba(0.92f, 0.95f, 0.98f, 0.92f);
    Color dim = color_rgba(0.62f, 0.70f, 0.78f, 0.85f);
    Bivector b = app_current_bivector(app);
    SplitBivector s = split_from_bivector(b);

    begin_overlay(app);

    snprintf(line, sizeof(line), "GA4D %s  OBJ:%s  VIEW:%s  BASIS:%s  %s",
             GA4D_VERSION,
             ga4d_object_names[app->object],
             ga4d_view_names[app->view],
             ga4d_basis_names[app->basis_mode],
             app->paused ? "PAUSED" : "RUN");
    draw_text_quads(14.0f, y, scale, line, text);
    y += 22.0f;

    if (app->basis_mode == BASIS_COORDINATE) {
        snprintf(line, sizeof(line),
                 "EDIT:%s  ANG:%+.2F  SPD:%+.2F   FOCAL:%+.2F  SLICE:%+.2F",
                 ga4d_plane_names[app->selected_coord],
                 bivector_get(&app->coord_angle, app->selected_coord),
                 bivector_get(&app->coord_speed, app->selected_coord),
                 app->focal4,
                 app->slice_w);
    } else {
        snprintf(line, sizeof(line),
                 "EDIT:%s  ANG:%+.2F  SPD:%+.2F   FOCAL:%+.2F  SLICE:%+.2F",
                 ga4d_split_names[app->selected_split],
                 split_get(&app->split_angle, app->selected_split),
                 split_get(&app->split_speed, app->selected_split),
                 app->focal4,
                 app->slice_w);
    }
    draw_text_quads(14.0f, y, scale, line, text);
    y += 22.0f;

    snprintf(line, sizeof(line),
             "B = %.2F E12  %.2F E13  %.2F E14  %.2F E23  %.2F E24  %.2F E34",
             b.e12, b.e13, b.e14, b.e23, b.e24, b.e34);
    draw_text_quads(14.0f, y, scale, line, dim);
    y += 20.0f;

    snprintf(line, sizeof(line),
             "HODGE SPLIT: L=(%.2F %.2F %.2F)  R=(%.2F %.2F %.2F)",
             s.l1, s.l2, s.l3, s.r1, s.r2, s.r3);
    draw_text_quads(14.0f, y, scale, line, dim);
    y += 22.0f;

    if (app->custom_icon_loaded || app->object == SHAPE_PNG_ICON) {
        snprintf(line, sizeof(line), "PNG ICON: %.220s",
                 app->custom_icon_loaded ? app->icon_status : "built-in sample; use --icon path.png");
        draw_text_quads(14.0f, y, scale, line, dim);
        y += 22.0f;
    }

    draw_text_quads(14.0f, y, scale,
                    "MOUSE: LEFT ORBIT  RIGHT PAN  WHEEL ZOOM   H HELP",
                    dim);
    y += 22.0f;

    if (app->show_help) {
        draw_text_quads(14.0f, y, scale,
                        "TAB OBJ  M VIEW  R BASIS  SPACE PAUSE  HOME RESET  ESC QUIT",
                        dim);
        y += 20.0f;
        draw_text_quads(14.0f, y, scale,
                        "1-6 SELECT GENERATOR  LEFT/RIGHT ANGLE  UP/DOWN SPEED  0 ZERO",
                        dim);
        y += 20.0f;
        draw_text_quads(14.0f, y, scale,
                        "[/] FOCAL  ,/. SLICE  Z/X ZOOM  WASD ORBIT  G GRID  V POINTS",
                        dim);
        y += 20.0f;
        draw_text_quads(14.0f, y, scale,
                        "F1 TESSERACT  F2 DOUBLE ROT  F3 L/R SPLIT  F4 SPHERE  F5 5-CELL",
                        dim);
        y += 20.0f;
        draw_text_quads(14.0f, y, scale,
                        "F6 ARROW-W  F7 PNG ICON-W  F8 3D HOUSE-W  CLI: --icon FILE.PNG",
                        dim);
    }

    end_overlay();
}

void render_scene(App *app)
{
    int fbw;
    int fbh;
    const Shape *shape = NULL;

    glfwGetFramebufferSize(app->window, &fbw, &fbh);
    app->width = fbw > 1 ? fbw : 1;
    app->height = fbh > 1 ? fbh : 1;

    glClearColor(0.055f, 0.060f, 0.068f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    begin_world(app);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    if (app->show_grid) {
        draw_grid();
    }

    draw_axes(app);

    glLineWidth(1.6f);
    if (app->object != SHAPE_HYPERSPHERE) {
        shape = &app->shapes[app->object];
        if (app->view == VIEW_PROJECT) {
            draw_shape_projection(app, shape, 0.94f);
        } else if (app->view == VIEW_SLICE) {
            draw_shape_slice(app, shape, 1.0f);
        } else {
            draw_shape_projection(app, shape, 0.25f);
            glLineWidth(2.2f);
            draw_shape_slice(app, shape, 1.0f);
        }
    } else {
        if (app->view == VIEW_PROJECT) {
            draw_hypersphere_projection(app, 0.80f);
        } else if (app->view == VIEW_SLICE) {
            draw_hypersphere_slice(app, 1.0f);
        } else {
            draw_hypersphere_projection(app, 0.22f);
            glLineWidth(2.2f);
            draw_hypersphere_slice(app, 1.0f);
        }
    }

    draw_overlay(app);
}
