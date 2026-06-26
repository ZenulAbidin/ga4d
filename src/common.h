#ifndef GA4D_COMMON_H
#define GA4D_COMMON_H

#define GA4D_PI 3.14159265358979323846f
#define GA4D_EPS 0.00001f
#define GA4D_VERSION "1.1.0"

#define MAX_VERTICES 4096
#define MAX_EDGES 16000
#define MAX_FACES 12000
#define MAX_FACE_VERTS 4
#define MAX_SLICE_POINTS 8
#define GA4D_PATH_MAX 512
#define GA4D_STATUS_MAX 256

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vec4;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} Color;

typedef struct {
    int a;
    int b;
} Edge;

typedef struct {
    int count;
    int v[MAX_FACE_VERTS];
} Face;

typedef struct {
    const char *name;
    int vertex_count;
    Vec4 vertices[MAX_VERTICES];
    int edge_count;
    Edge edges[MAX_EDGES];
    int face_count;
    Face faces[MAX_FACES];
} Shape;

enum {
    SHAPE_TESSERACT = 0,
    SHAPE_CELL16,
    SHAPE_SIMPLEX5,
    SHAPE_HYPERSPHERE,
    SHAPE_ICON_ARROW,
    SHAPE_ICON_STAR,
    SHAPE_PNG_ICON,
    SHAPE_HOUSE_W,
    SHAPE_COUNT
};

enum {
    VIEW_PROJECT = 0,
    VIEW_SLICE,
    VIEW_BOTH,
    VIEW_COUNT
};

enum {
    BASIS_COORDINATE = 0,
    BASIS_SELF_DUAL,
    BASIS_COUNT
};

enum {
    PLANE_E12 = 0,
    PLANE_E13,
    PLANE_E14,
    PLANE_E23,
    PLANE_E24,
    PLANE_E34,
    PLANE_COUNT
};

enum {
    SPLIT_L1 = 0,
    SPLIT_L2,
    SPLIT_L3,
    SPLIT_R1,
    SPLIT_R2,
    SPLIT_R3,
    SPLIT_COUNT
};

extern const char *ga4d_plane_names[PLANE_COUNT];
extern const char *ga4d_split_names[SPLIT_COUNT];
extern const char *ga4d_view_names[VIEW_COUNT];
extern const char *ga4d_object_names[SHAPE_COUNT];
extern const char *ga4d_basis_names[BASIS_COUNT];

#endif
