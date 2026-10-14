#pragma once
#include <stdbool.h>
typedef struct { int left, top, right, bottom; } YoshiInsets;
typedef struct {
  int x, y, width, height;
  double pixel_aspect, scale;
  int logical_width, logical_height;
  double original_left, original_width;
} YoshiDisplayGeometry;
/* Geometry only: this does not claim the renderer supports the expanded width.
 * Caller supplies WindowInsets and the live surface size, both in pixels.
 * pixel_aspect is explicit (7/6 for the framework's CRT correction; 1 square).
 */
bool yoshi_display_geometry(int width, int height, YoshiInsets insets,
                            double pixel_aspect, YoshiDisplayGeometry *out);
