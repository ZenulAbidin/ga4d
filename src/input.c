#include "input.h"

#include <math.h>
#include <stdio.h>

static int key_once(App *app, int key)
{
    int down;
    int pressed;

    if (key < 0 || key > GLFW_KEY_LAST) {
        return 0;
    }

    down = glfwGetKey(app->window, key) == GLFW_PRESS;
    pressed = down && !app->prev_keys[key];
    app->prev_keys[key] = down;
    return pressed;
}

static int key_down(App *app, int key)
{
    return glfwGetKey(app->window, key) == GLFW_PRESS;
}

static void zero_selected(App *app)
{
    if (app->basis_mode == BASIS_COORDINATE) {
        bivector_set(&app->coord_angle, app->selected_coord, 0.0f);
        bivector_set(&app->coord_speed, app->selected_coord, 0.0f);
    } else {
        split_set(&app->split_angle, app->selected_split, 0.0f);
        split_set(&app->split_speed, app->selected_split, 0.0f);
    }
    app_update_rotor(app);
}

static void adjust_selected_angle(App *app, float delta)
{
    if (app->basis_mode == BASIS_COORDINATE) {
        float v = bivector_get(&app->coord_angle, app->selected_coord);
        bivector_set(&app->coord_angle, app->selected_coord, v + delta);
    } else {
        float v = split_get(&app->split_angle, app->selected_split);
        split_set(&app->split_angle, app->selected_split, v + delta);
    }
    app_update_rotor(app);
}

static void adjust_selected_speed(App *app, float delta)
{
    if (app->basis_mode == BASIS_COORDINATE) {
        float v = bivector_get(&app->coord_speed, app->selected_coord);
        bivector_set(&app->coord_speed, app->selected_coord, clampf(v + delta, -3.0f, 3.0f));
    } else {
        float v = split_get(&app->split_speed, app->selected_split);
        split_set(&app->split_speed, app->selected_split, clampf(v + delta, -3.0f, 3.0f));
    }
}

void update_title(App *app)
{
    char title[256];

    snprintf(title, sizeof(title),
             "ga4d %s - %s / %s / %s - H toggles help",
             GA4D_VERSION,
             ga4d_object_names[app->object],
             ga4d_view_names[app->view],
             ga4d_basis_names[app->basis_mode]);
    glfwSetWindowTitle(app->window, title);
}

void process_input(App *app, float dt)
{
    int i;

    if (key_once(app, GLFW_KEY_ESCAPE) || key_once(app, GLFW_KEY_Q)) {
        glfwSetWindowShouldClose(app->window, GLFW_TRUE);
    }
    if (key_once(app, GLFW_KEY_H)) {
        app->show_help = !app->show_help;
    }
    if (key_once(app, GLFW_KEY_G)) {
        app->show_grid = !app->show_grid;
    }
    if (key_once(app, GLFW_KEY_V)) {
        app->show_points = !app->show_points;
    }
    if (key_once(app, GLFW_KEY_C)) {
        app->draw_depth_cue = !app->draw_depth_cue;
    }
    if (key_once(app, GLFW_KEY_SPACE)) {
        app->paused = !app->paused;
    }
    if (key_once(app, GLFW_KEY_HOME)) {
        app_reset_view(app);
        update_title(app);
    }
    if (key_once(app, GLFW_KEY_TAB)) {
        int backwards = key_down(app, GLFW_KEY_LEFT_SHIFT) || key_down(app, GLFW_KEY_RIGHT_SHIFT);
        app->object += backwards ? -1 : 1;
        if (app->object < 0) {
            app->object = SHAPE_COUNT - 1;
        }
        app->object %= SHAPE_COUNT;
        update_title(app);
    }
    if (key_once(app, GLFW_KEY_M)) {
        app->view = (app->view + 1) % VIEW_COUNT;
        update_title(app);
    }
    if (key_once(app, GLFW_KEY_R)) {
        app->basis_mode = (app->basis_mode + 1) % BASIS_COUNT;
        app_update_rotor(app);
        update_title(app);
    }

    for (i = 0; i < 6; ++i) {
        if (key_once(app, GLFW_KEY_1 + i)) {
            if (app->basis_mode == BASIS_COORDINATE) {
                app->selected_coord = i;
            } else {
                app->selected_split = i;
            }
        }
    }
    if (key_once(app, GLFW_KEY_0)) {
        zero_selected(app);
    }

    if (key_once(app, GLFW_KEY_F1)) {
        app_apply_preset(app, 1);
        update_title(app);
    }
    if (key_once(app, GLFW_KEY_F2)) {
        app_apply_preset(app, 2);
        update_title(app);
    }
    if (key_once(app, GLFW_KEY_F3)) {
        app_apply_preset(app, 3);
        update_title(app);
    }
    if (key_once(app, GLFW_KEY_F4)) {
        app_apply_preset(app, 4);
        update_title(app);
    }
    if (key_once(app, GLFW_KEY_F5)) {
        app_apply_preset(app, 5);
        update_title(app);
    }
    if (key_once(app, GLFW_KEY_F6)) {
        app_apply_preset(app, 6);
        update_title(app);
    }
    if (key_once(app, GLFW_KEY_F7)) {
        app_apply_preset(app, 7);
        update_title(app);
    }
    if (key_once(app, GLFW_KEY_F8)) {
        app_apply_preset(app, 8);
        update_title(app);
    }

    if (key_down(app, GLFW_KEY_LEFT)) {
        adjust_selected_angle(app, -1.35f * dt);
    }
    if (key_down(app, GLFW_KEY_RIGHT)) {
        adjust_selected_angle(app, 1.35f * dt);
    }
    if (key_down(app, GLFW_KEY_UP)) {
        adjust_selected_speed(app, 0.70f * dt);
    }
    if (key_down(app, GLFW_KEY_DOWN)) {
        adjust_selected_speed(app, -0.70f * dt);
    }
    if (key_down(app, GLFW_KEY_LEFT_BRACKET)) {
        app->focal4 -= 1.15f * dt;
    }
    if (key_down(app, GLFW_KEY_RIGHT_BRACKET)) {
        app->focal4 += 1.15f * dt;
    }
    if (key_down(app, GLFW_KEY_COMMA)) {
        app->slice_w -= 1.10f * dt;
    }
    if (key_down(app, GLFW_KEY_PERIOD)) {
        app->slice_w += 1.10f * dt;
    }
    if (key_down(app, GLFW_KEY_Z)) {
        app->camera_distance *= expf(1.4f * dt);
    }
    if (key_down(app, GLFW_KEY_X)) {
        app->camera_distance *= expf(-1.4f * dt);
    }
    if (key_down(app, GLFW_KEY_A)) {
        app->camera_yaw -= 55.0f * dt;
    }
    if (key_down(app, GLFW_KEY_D)) {
        app->camera_yaw += 55.0f * dt;
    }
    if (key_down(app, GLFW_KEY_W)) {
        app->camera_pitch -= 55.0f * dt;
    }
    if (key_down(app, GLFW_KEY_S)) {
        app->camera_pitch += 55.0f * dt;
    }

    app->focal4 = clampf(app->focal4, 2.35f, 12.0f);
    app->slice_w = clampf(app->slice_w, -1.9f, 1.9f);
    app->camera_distance = clampf(app->camera_distance, 2.0f, 40.0f);
    app->camera_pitch = clampf(app->camera_pitch, -89.0f, 89.0f);
}

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    App *app = (App *)glfwGetWindowUserPointer(window);
    (void)mods;

    if (!app) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        app->mouse_left = action == GLFW_PRESS;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        app->mouse_right = action == GLFW_PRESS;
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        app->mouse_middle = action == GLFW_PRESS;
    }
    glfwGetCursorPos(window, &app->last_mouse_x, &app->last_mouse_y);
}

static void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
    App *app = (App *)glfwGetWindowUserPointer(window);
    double dx;
    double dy;

    if (!app) {
        return;
    }

    dx = x - app->last_mouse_x;
    dy = y - app->last_mouse_y;
    app->last_mouse_x = x;
    app->last_mouse_y = y;

    if (app->mouse_left) {
        app->camera_yaw += (float)dx * 0.32f;
        app->camera_pitch += (float)dy * 0.32f;
        app->camera_pitch = clampf(app->camera_pitch, -89.0f, 89.0f);
    } else if (app->mouse_right || app->mouse_middle) {
        float scale = 0.0026f * app->camera_distance;
        app->pan_x += (float)dx * scale;
        app->pan_y -= (float)dy * scale;
    }
}

static void scroll_callback(GLFWwindow *window, double xoff, double yoff)
{
    App *app = (App *)glfwGetWindowUserPointer(window);
    (void)xoff;

    if (!app) {
        return;
    }

    app->camera_distance *= expf((float)-yoff * 0.12f);
    app->camera_distance = clampf(app->camera_distance, 2.0f, 40.0f);
}

void install_input_callbacks(App *app)
{
    glfwSetWindowUserPointer(app->window, app);
    glfwSetMouseButtonCallback(app->window, mouse_button_callback);
    glfwSetCursorPosCallback(app->window, cursor_pos_callback);
    glfwSetScrollCallback(app->window, scroll_callback);
}
