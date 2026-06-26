#ifndef GA4D_APP_H
#define GA4D_APP_H

#include <GLFW/glfw3.h>

#include "common.h"
#include "math4d.h"

typedef struct {
    GLFWwindow *window;
    int width;
    int height;

    Shape shapes[SHAPE_COUNT];
    int object;
    int view;
    int basis_mode;
    int selected_coord;
    int selected_split;

    Bivector coord_angle;
    Bivector coord_speed;
    SplitBivector split_angle;
    SplitBivector split_speed;
    Multivector rotor;

    float focal4;
    float slice_w;
    float camera_distance;
    float camera_yaw;
    float camera_pitch;
    float pan_x;
    float pan_y;

    int paused;
    int show_help;
    int show_grid;
    int show_points;
    int draw_depth_cue;

    int mouse_left;
    int mouse_right;
    int mouse_middle;
    double last_mouse_x;
    double last_mouse_y;
    int prev_keys[GLFW_KEY_LAST + 1];
    double title_timer;

    int custom_icon_loaded;
    char custom_icon_path[GA4D_PATH_MAX];
    char icon_status[GA4D_STATUS_MAX];
} App;

void app_defaults(App *app);
void app_reset_view(App *app);
void app_apply_preset(App *app, int preset);
int app_load_png_icon(App *app, const char *path);
void app_update_rotor(App *app);
void app_update_animation(App *app, float dt);
Bivector app_current_bivector(const App *app);
Vec4 app_rotate_vec4(const App *app, Vec4 p);

#endif
