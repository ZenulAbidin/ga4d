#include "app.h"

#include "extrude.h"
#include "shapes.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void set_coord_component(Bivector *b, int plane, float value)
{
    bivector_set(b, plane, value);
}

static void set_split_component(SplitBivector *s, int index, float value)
{
    split_set(s, index, value);
}

static void copy_with_prefix(char *dst, size_t dst_size, const char *prefix, const char *text)
{
    size_t n = 0;

    if (dst_size == 0) {
        return;
    }
    while (prefix && *prefix && n + 1 < dst_size) {
        dst[n++] = *prefix++;
    }
    while (text && *text && n + 1 < dst_size) {
        dst[n++] = *text++;
    }
    dst[n] = '\0';
}

Bivector app_current_bivector(const App *app)
{
    if (app->basis_mode == BASIS_SELF_DUAL) {
        return bivector_from_split(app->split_angle);
    }
    return app->coord_angle;
}

void app_update_rotor(App *app)
{
    Bivector b = app_current_bivector(app);
    app->rotor = rotor_from_bivector_sequence(b);
}

Vec4 app_rotate_vec4(const App *app, Vec4 p)
{
    return rotor_apply_vec4(app->rotor, p);
}

int app_load_png_icon(App *app, const char *path)
{
    char status[GA4D_STATUS_MAX];

    if (!build_png_icon_shape(&app->shapes[SHAPE_PNG_ICON],
                              path, status, sizeof(status))) {
        copy_with_prefix(app->icon_status, sizeof(app->icon_status),
                         "PNG icon failed: ", status);
        app->custom_icon_loaded = 0;
        app->custom_icon_path[0] = '\0';
        return 0;
    }

    snprintf(app->custom_icon_path, sizeof(app->custom_icon_path), "%s", path);
    snprintf(app->icon_status, sizeof(app->icon_status), "%s", status);
    app->custom_icon_loaded = 1;
    return 1;
}

void app_defaults(App *app)
{
    memset(app, 0, sizeof(*app));
    app->width = 1280;
    app->height = 800;
    app->object = SHAPE_TESSERACT;
    app->view = VIEW_BOTH;
    app->basis_mode = BASIS_COORDINATE;
    app->selected_coord = PLANE_E14;
    app->selected_split = SPLIT_L1;

    app->coord_angle = bivector_zero();
    app->coord_speed = bivector_zero();
    set_coord_component(&app->coord_angle, PLANE_E14, 0.28f);
    set_coord_component(&app->coord_speed, PLANE_E14, 0.48f);
    set_coord_component(&app->coord_speed, PLANE_E24, 0.19f);
    set_coord_component(&app->coord_speed, PLANE_E34, -0.11f);

    memset(&app->split_angle, 0, sizeof(app->split_angle));
    memset(&app->split_speed, 0, sizeof(app->split_speed));
    set_split_component(&app->split_speed, SPLIT_L1, 0.35f);
    set_split_component(&app->split_speed, SPLIT_R1, -0.12f);

    app->focal4 = 4.0f;
    app->slice_w = 0.0f;
    app->camera_distance = 7.0f;
    app->camera_yaw = -28.0f;
    app->camera_pitch = 22.0f;
    app->show_help = 1;
    app->show_grid = 1;
    app->show_points = 1;
    app->draw_depth_cue = 1;

    build_shapes(app->shapes);
    app_update_rotor(app);
}

void app_reset_view(App *app)
{
    int old_help = app->show_help;
    int old_grid = app->show_grid;
    int old_points = app->show_points;
    int old_depth = app->draw_depth_cue;
    int old_custom_icon_loaded = app->custom_icon_loaded;
    char old_custom_icon_path[GA4D_PATH_MAX];
    char old_icon_status[GA4D_STATUS_MAX];
    Shape old_png_icon = app->shapes[SHAPE_PNG_ICON];
    GLFWwindow *window = app->window;

    snprintf(old_custom_icon_path, sizeof(old_custom_icon_path), "%s", app->custom_icon_path);
    snprintf(old_icon_status, sizeof(old_icon_status), "%s", app->icon_status);

    app_defaults(app);
    app->window = window;
    app->show_help = old_help;
    app->show_grid = old_grid;
    app->show_points = old_points;
    app->draw_depth_cue = old_depth;
    if (old_custom_icon_loaded) {
        app->shapes[SHAPE_PNG_ICON] = old_png_icon;
        app->custom_icon_loaded = old_custom_icon_loaded;
        snprintf(app->custom_icon_path, sizeof(app->custom_icon_path), "%s", old_custom_icon_path);
        snprintf(app->icon_status, sizeof(app->icon_status), "%s", old_icon_status);
    }
}

void app_apply_preset(App *app, int preset)
{
    app->coord_angle = bivector_zero();
    app->coord_speed = bivector_zero();
    memset(&app->split_angle, 0, sizeof(app->split_angle));
    memset(&app->split_speed, 0, sizeof(app->split_speed));
    app->slice_w = 0.0f;

    if (preset == 1) {
        app->object = SHAPE_TESSERACT;
        app->view = VIEW_BOTH;
        app->basis_mode = BASIS_COORDINATE;
        app->selected_coord = PLANE_E14;
        set_coord_component(&app->coord_speed, PLANE_E14, 0.55f);
        set_coord_component(&app->coord_speed, PLANE_E24, 0.18f);
        set_coord_component(&app->coord_speed, PLANE_E34, -0.13f);
        app->focal4 = 4.0f;
    } else if (preset == 2) {
        app->object = SHAPE_TESSERACT;
        app->view = VIEW_PROJECT;
        app->basis_mode = BASIS_COORDINATE;
        app->selected_coord = PLANE_E12;
        set_coord_component(&app->coord_speed, PLANE_E12, 0.55f);
        set_coord_component(&app->coord_speed, PLANE_E34, 0.95f);
        app->focal4 = 4.2f;
    } else if (preset == 3) {
        app->object = SHAPE_CELL16;
        app->view = VIEW_BOTH;
        app->basis_mode = BASIS_SELF_DUAL;
        app->selected_split = SPLIT_L1;
        set_split_component(&app->split_speed, SPLIT_L1, 0.50f);
        set_split_component(&app->split_speed, SPLIT_R1, -0.20f);
        app->focal4 = 4.0f;
    } else if (preset == 4) {
        app->object = SHAPE_HYPERSPHERE;
        app->view = VIEW_BOTH;
        app->basis_mode = BASIS_COORDINATE;
        app->selected_coord = PLANE_E14;
        set_coord_component(&app->coord_speed, PLANE_E14, 0.28f);
        set_coord_component(&app->coord_speed, PLANE_E23, 0.12f);
        app->slice_w = -0.45f;
        app->focal4 = 3.7f;
    } else if (preset == 5) {
        app->object = SHAPE_SIMPLEX5;
        app->view = VIEW_BOTH;
        app->basis_mode = BASIS_SELF_DUAL;
        app->selected_split = SPLIT_L2;
        set_split_component(&app->split_speed, SPLIT_L2, 0.40f);
        set_split_component(&app->split_speed, SPLIT_R3, 0.31f);
        app->focal4 = 4.4f;
    } else if (preset == 6) {
        app->object = SHAPE_ICON_ARROW;
        app->view = VIEW_BOTH;
        app->basis_mode = BASIS_COORDINATE;
        app->selected_coord = PLANE_E14;
        set_coord_component(&app->coord_angle, PLANE_E13, 0.34f);
        set_coord_component(&app->coord_angle, PLANE_E14, 0.48f);
        set_coord_component(&app->coord_speed, PLANE_E14, 0.58f);
        set_coord_component(&app->coord_speed, PLANE_E24, -0.16f);
        app->focal4 = 4.1f;
    } else if (preset == 7) {
        app->object = SHAPE_PNG_ICON;
        app->view = VIEW_BOTH;
        app->basis_mode = BASIS_COORDINATE;
        app->selected_coord = PLANE_E14;
        set_coord_component(&app->coord_angle, PLANE_E13, -0.26f);
        set_coord_component(&app->coord_angle, PLANE_E14, 0.52f);
        set_coord_component(&app->coord_speed, PLANE_E14, 0.50f);
        set_coord_component(&app->coord_speed, PLANE_E12, 0.18f);
        app->focal4 = 4.0f;
    } else if (preset == 8) {
        app->object = SHAPE_HOUSE_W;
        app->view = VIEW_BOTH;
        app->basis_mode = BASIS_COORDINATE;
        app->selected_coord = PLANE_E34;
        set_coord_component(&app->coord_speed, PLANE_E14, 0.28f);
        set_coord_component(&app->coord_speed, PLANE_E24, 0.24f);
        set_coord_component(&app->coord_speed, PLANE_E34, 0.42f);
        app->focal4 = 4.3f;
    }

    app_update_rotor(app);
}

void app_update_animation(App *app, float dt)
{
    int i;

    if (app->paused) {
        return;
    }

    if (app->basis_mode == BASIS_COORDINATE) {
        for (i = 0; i < PLANE_COUNT; ++i) {
            float a = bivector_get(&app->coord_angle, i);
            float s = bivector_get(&app->coord_speed, i);
            a += s * dt;
            if (a > GA4D_PI * 8.0f || a < -GA4D_PI * 8.0f) {
                a = fmodf(a, GA4D_PI * 2.0f);
            }
            bivector_set(&app->coord_angle, i, a);
        }
    } else {
        for (i = 0; i < SPLIT_COUNT; ++i) {
            float a = split_get(&app->split_angle, i);
            float s = split_get(&app->split_speed, i);
            a += s * dt;
            if (a > GA4D_PI * 8.0f || a < -GA4D_PI * 8.0f) {
                a = fmodf(a, GA4D_PI * 2.0f);
            }
            split_set(&app->split_angle, i, a);
        }
    }

    app_update_rotor(app);
}
