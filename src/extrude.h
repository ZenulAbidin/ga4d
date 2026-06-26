#ifndef GA4D_EXTRUDE_H
#define GA4D_EXTRUDE_H

#include "common.h"

#include <stddef.h>

void build_icon_arrow_shape(Shape *shape);
void build_icon_star_shape(Shape *shape);
void build_png_sample_shape(Shape *shape);
void build_house_w_shape(Shape *shape);
int build_png_icon_shape(Shape *shape, const char *path, char *status, size_t status_size);

#endif
