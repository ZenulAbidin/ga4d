#include "app.h"

#include "shapes.h"

#include <math.h>
#include <string.h>

static void set_coord_component(Bivector *b, int plane, float value)
{
    bivector_set(b, plane, value);
}

static void set_split_component(SplitBivector *s, int index, float value)
{
    split_set(s, index, value);
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
    GLFWwindow *window = app->window;

    app_defaults(app);
    app->window = window;
    app->show_help = old_help;
    app->show_grid = old_grid;
    app->show_points = old_points;
    app->draw_depth_cue = old_depth;
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
