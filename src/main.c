#define _POSIX_C_SOURCE 200112L

#include <GLFW/glfw3.h>

#include "app.h"
#include "input.h"
#include "math4d.h"
#include "render.h"
#include "validate.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

typedef struct {
    unsigned char *pixels;
    int capacity;
    int non_background_frames;
    int checked_frames;
} SmokeStats;

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW error %d: %s\n", error, description ? description : "(no description)");
}

static void smoke_stats_free(SmokeStats *stats)
{
    free(stats->pixels);
    stats->pixels = NULL;
    stats->capacity = 0;
}

static int smoke_check_frame(GLFWwindow *window, SmokeStats *stats)
{
    int width;
    int height;
    int needed;
    int i;
    int bright_pixels = 0;
    int varied_pixels;
    unsigned char min_r = 255;
    unsigned char max_r = 0;
    unsigned char min_g = 255;
    unsigned char max_g = 0;
    unsigned char min_b = 255;
    unsigned char max_b = 0;
    GLenum err;

    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "render smoke: invalid framebuffer size %dx%d\n", width, height);
        return 0;
    }

    needed = width * height * 3;
    if (needed <= 0) {
        fprintf(stderr, "render smoke: invalid framebuffer allocation size\n");
        return 0;
    }

    if (needed > stats->capacity) {
        unsigned char *new_pixels = (unsigned char *)realloc(stats->pixels, (size_t)needed);
        if (!new_pixels) {
            fprintf(stderr, "render smoke: framebuffer allocation failed\n");
            return 0;
        }
        stats->pixels = new_pixels;
        stats->capacity = needed;
    }

    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, stats->pixels);
    err = glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "render smoke: OpenGL error 0x%04x after glReadPixels\n", (unsigned)err);
        return 0;
    }

    for (i = 0; i < needed; i += 3) {
        unsigned char r = stats->pixels[i + 0];
        unsigned char g = stats->pixels[i + 1];
        unsigned char b = stats->pixels[i + 2];
        int brightness = (int)r + (int)g + (int)b;

        if (r < min_r) {
            min_r = r;
        }
        if (r > max_r) {
            max_r = r;
        }
        if (g < min_g) {
            min_g = g;
        }
        if (g > max_g) {
            max_g = g;
        }
        if (b < min_b) {
            min_b = b;
        }
        if (b > max_b) {
            max_b = b;
        }
        if (brightness > 90) {
            bright_pixels++;
        }
    }

    varied_pixels = ((int)max_r - (int)min_r) +
                    ((int)max_g - (int)min_g) +
                    ((int)max_b - (int)min_b);

    stats->checked_frames++;
    if (bright_pixels > 12 && varied_pixels > 20) {
        stats->non_background_frames++;
    }

    return 1;
}

static int smoke_stats_passed(const SmokeStats *stats)
{
    return stats->checked_frames > 0 && stats->non_background_frames > 0;
}

static void print_usage(const char *argv0)
{
    printf("usage: %s [--self-test] [--validate-visuals] [--smoke-frames N] [--headless-smoke N] [--help]\n", argv0);
    printf("\n");
    printf("Interactive controls are shown in the viewer overlay.\n");
}

static int test_topology(const App *app)
{
    if (app->shapes[SHAPE_TESSERACT].vertex_count != 16 ||
        app->shapes[SHAPE_TESSERACT].edge_count != 32 ||
        app->shapes[SHAPE_TESSERACT].face_count != 24) {
        fprintf(stderr, "self-test: tesseract topology mismatch\n");
        return 0;
    }

    if (app->shapes[SHAPE_CELL16].vertex_count != 8 ||
        app->shapes[SHAPE_CELL16].edge_count != 24 ||
        app->shapes[SHAPE_CELL16].face_count != 32) {
        fprintf(stderr, "self-test: 16-cell topology mismatch\n");
        return 0;
    }

    if (app->shapes[SHAPE_SIMPLEX5].vertex_count != 5 ||
        app->shapes[SHAPE_SIMPLEX5].edge_count != 10 ||
        app->shapes[SHAPE_SIMPLEX5].face_count != 10) {
        fprintf(stderr, "self-test: 5-cell topology mismatch\n");
        return 0;
    }

    return 1;
}

static int test_split_round_trip(void)
{
    Bivector b;
    Bivector back;
    SplitBivector s;
    float err;

    b.e12 = 0.10f;
    b.e13 = -0.20f;
    b.e14 = 0.30f;
    b.e23 = 0.40f;
    b.e24 = -0.50f;
    b.e34 = 0.60f;

    s = split_from_bivector(b);
    back = bivector_from_split(s);

    err = fabsf(b.e12 - back.e12) + fabsf(b.e13 - back.e13) +
          fabsf(b.e14 - back.e14) + fabsf(b.e23 - back.e23) +
          fabsf(b.e24 - back.e24) + fabsf(b.e34 - back.e34);

    if (err > 0.00001f) {
        fprintf(stderr, "self-test: self/anti-dual split round trip failed\n");
        return 0;
    }

    return 1;
}

static int test_rotor_preserves_length(void)
{
    Bivector b;
    Multivector r;
    Vec4 p;
    Vec4 out;
    float before;
    float after;

    b.e12 = 0.41f;
    b.e13 = -0.17f;
    b.e14 = 0.72f;
    b.e23 = 0.11f;
    b.e24 = -0.29f;
    b.e34 = 0.53f;

    r = rotor_from_bivector_sequence(b);
    p = vec4(0.7f, -0.2f, 1.1f, 0.4f);
    out = rotor_apply_vec4(r, p);
    before = vec4_len(p);
    after = vec4_len(out);

    if (fabsf(before - after) > 0.0002f) {
        fprintf(stderr, "self-test: rotor changed vector length (%f -> %f)\n",
                before, after);
        return 0;
    }

    return 1;
}

static int test_projection(App *app)
{
    int obj;
    int i;

    app->coord_angle.e12 = 0.23f;
    app->coord_angle.e13 = -0.41f;
    app->coord_angle.e14 = 0.72f;
    app->coord_angle.e23 = 0.11f;
    app->coord_angle.e24 = -0.18f;
    app->coord_angle.e34 = 0.51f;
    app_update_rotor(app);

    for (obj = 0; obj < 3; ++obj) {
        const Shape *shape = &app->shapes[obj];
        for (i = 0; i < shape->vertex_count; ++i) {
            Vec3 out;
            Vec4 p = app_rotate_vec4(app, shape->vertices[i]);
            if (!project4_to3(app, p, &out)) {
                fprintf(stderr, "self-test: projection rejected vertex %d of %s\n",
                        i, shape->name);
                return 0;
            }
            if (!finite_float(out.x) || !finite_float(out.y) || !finite_float(out.z)) {
                fprintf(stderr, "self-test: non-finite projection for %s\n", shape->name);
                return 0;
            }
        }
    }

    return 1;
}

static int run_self_tests(void)
{
    App app;

    app_defaults(&app);
    if (!test_topology(&app) ||
        !test_split_round_trip() ||
        !test_rotor_preserves_length() ||
        !test_projection(&app)) {
        return 1;
    }

    printf("ga4d self-test passed\n");
    return 0;
}

static int parse_frame_arg(int argc, char **argv, const char *name)
{
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], name) == 0) {
            if (i + 1 >= argc) {
                return -1;
            }
            return atoi(argv[i + 1]);
        }
    }

    return 0;
}

static int has_arg(int argc, char **argv, const char *name)
{
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], name) == 0) {
            return 1;
        }
    }

    return 0;
}

static int run_viewer(int smoke_frames, int headless)
{
    App app;
    SmokeStats smoke_stats;
    const char *platform_hint;
    double last_time;
    int frames = 0;
    int smoke_ok = 1;

    memset(&smoke_stats, 0, sizeof(smoke_stats));
    app_defaults(&app);

    glfwSetErrorCallback(glfw_error_callback);

    platform_hint = getenv("GA4D_GLFW_PLATFORM");
    if (platform_hint && strcmp(platform_hint, "wayland") == 0) {
#if defined(GLFW_PLATFORM) && defined(GLFW_PLATFORM_WAYLAND)
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
#else
        fprintf(stderr, "GLFW Wayland platform hint is not supported by these headers\n");
        return 1;
#endif
    } else if (platform_hint && strcmp(platform_hint, "x11") == 0) {
#if defined(GLFW_PLATFORM) && defined(GLFW_PLATFORM_X11)
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#else
        fprintf(stderr, "GLFW X11 platform hint is not supported by these headers\n");
        return 1;
#endif
    } else if (platform_hint && strcmp(platform_hint, "null") == 0) {
#if defined(GLFW_PLATFORM) && defined(GLFW_PLATFORM_NULL)
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_NULL);
#else
        fprintf(stderr, "GLFW null platform hint is not supported by these headers\n");
        return 1;
#endif
    } else if (platform_hint && platform_hint[0] != '\0') {
        fprintf(stderr, "unknown GA4D_GLFW_PLATFORM value: %s\n", platform_hint);
        return 1;
    }

    if (headless) {
#if defined(GLFW_PLATFORM) && defined(GLFW_PLATFORM_NULL)
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_NULL);
        if (getenv("EGL_PLATFORM") == NULL) {
            setenv("EGL_PLATFORM", "surfaceless", 0);
        }
#else
        fprintf(stderr, "headless smoke requires GLFW 3.4 platform selection support\n");
        return 1;
#endif
    }

    if (!glfwInit()) {
        fprintf(stderr, "failed to initialize GLFW; check display/driver availability\n");
        return 1;
    }

    glfwWindowHint(GLFW_SAMPLES, headless ? 0 : 4);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    if (headless) {
#if defined(GLFW_CONTEXT_CREATION_API) && defined(GLFW_EGL_CONTEXT_API)
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#endif
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    app.window = glfwCreateWindow(app.width, app.height,
                                  "ga4d - 4D Clifford geometry viewer",
                                  NULL, NULL);
    if (!app.window) {
        fprintf(stderr, "failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(app.window);
    glfwSwapInterval(1);
    install_input_callbacks(&app);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glPointSize(4.0f);
    update_title(&app);

    printf("ga4d controls: H help, Tab object, M view, R basis, Space pause, Esc quit\n");

    last_time = glfwGetTime();
    while (!glfwWindowShouldClose(app.window)) {
        double now = glfwGetTime();
        float dt = (float)(now - last_time);
        last_time = now;

        if (!finite_float(dt) || dt < 0.0f) {
            dt = 0.0f;
        }
        if (dt > 0.05f) {
            dt = 0.05f;
        }

        glfwPollEvents();
        process_input(&app, dt);
        app_update_animation(&app, dt);
        render_scene(&app);
        if (smoke_frames > 0) {
            if (!smoke_check_frame(app.window, &smoke_stats)) {
                smoke_ok = 0;
                break;
            }
        }
        glfwSwapBuffers(app.window);

        app.title_timer += dt;
        if (app.title_timer > 0.5) {
            update_title(&app);
            app.title_timer = 0.0;
        }

        if (smoke_frames > 0) {
            frames++;
            if (frames >= smoke_frames) {
                break;
            }
        }
    }

    if (smoke_frames > 0 && smoke_ok && !smoke_stats_passed(&smoke_stats)) {
        fprintf(stderr, "render smoke: framebuffer looked blank across %d checked frame(s)\n",
                smoke_stats.checked_frames);
        smoke_ok = 0;
    }
    if (smoke_frames > 0 && smoke_ok) {
        printf("ga4d render smoke passed (%d frame(s), %d nonblank)\n",
               smoke_stats.checked_frames,
               smoke_stats.non_background_frames);
    }

    smoke_stats_free(&smoke_stats);
    glfwDestroyWindow(app.window);
    glfwTerminate();
    return smoke_ok ? 0 : 1;
}

int main(int argc, char **argv)
{
    int smoke_frames;
    int headless_frames;

    if (argc > 1) {
        if (strcmp(argv[1], "--self-test") == 0) {
            return run_self_tests();
        }
        if (strcmp(argv[1], "--validate-visuals") == 0) {
            return run_visual_validation();
        }
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    smoke_frames = parse_frame_arg(argc, argv, "--smoke-frames");
    if (smoke_frames < 0) {
        fprintf(stderr, "--smoke-frames requires a positive frame count\n");
        return 2;
    }
    headless_frames = parse_frame_arg(argc, argv, "--headless-smoke");
    if (headless_frames < 0) {
        fprintf(stderr, "--headless-smoke requires a positive frame count\n");
        return 2;
    }

    if (headless_frames > 0) {
        return run_viewer(headless_frames, 1);
    }

    if (argc > 1 && smoke_frames == 0) {
        fprintf(stderr, "unknown argument: %s\n", argv[1]);
        print_usage(argv[0]);
        return 2;
    }

    if (has_arg(argc, argv, "--smoke-frames")) {
        return run_viewer(smoke_frames, 0);
    }

    return run_viewer(0, 0);
}
