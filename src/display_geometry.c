#include "display_geometry.h"
#include <math.h>
#include <limits.h>
bool yoshi_display_geometry(int width, int height, YoshiInsets insets,
                            double par, YoshiDisplayGeometry *out) {
  if (!out || width <= 0 || height <= 0 || !isfinite(par) || par <= 0 ||
      insets.left < 0 || insets.right < 0 || insets.top < 0 || insets.bottom < 0)
    return false;
  long long w = (long long)width - insets.left - insets.right;
  long long h = (long long)height - insets.top - insets.bottom;
  if (w <= 0 || h <= 0) return false;
  double scale = (double)h / 224.0;
  double logical = (double)w / (scale * par);
  if (!isfinite(logical) || logical > INT_MAX || logical < 1) return false;
  *out = (YoshiDisplayGeometry){insets.left, insets.top, (int)w, (int)h,
    par, scale, (int)ceil(logical), 224,
    ((double)w - 256.0 * scale * par) / 2.0, 256.0 * scale * par};
  return true;
}
bool yoshi_adaptive_viewport(int frame_width, int frame_height,
                             int drawable_width, int drawable_height,
                             YoshiViewport *out) {
  if (!out || frame_width <= 256 || frame_height != 224 ||
      drawable_width <= 0 || drawable_height <= 0) return false;
  double scale = (double)drawable_height / frame_height;
  double full = frame_width * (7.0 / 6.0) * scale;
  double surplus = full - drawable_width;
  /* Fixed test fields, native fallback and OAM-capped ultra-wide fields keep
   * the framework viewport. Never crop real gameplay to fill a narrower screen. */
  if (!isfinite(full) || full > INT_MAX || surplus < 0 ||
      surplus >= 2.0 * (7.0 / 6.0) * scale + 0.001) return false;
  int width = (int)ceil(full);
  *out = (YoshiViewport){(drawable_width - width) / 2, 0, width, drawable_height};
  return true;
}
