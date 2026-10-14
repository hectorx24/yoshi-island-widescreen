#include "display_geometry.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void) {
  int sizes[][2] = {{1920,1080},{2340,1080},{2400,1080},{2520,1080},{2772,1272}};
  for (unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++) {
    YoshiDisplayGeometry g;
    assert(yoshi_display_geometry(sizes[i][0],sizes[i][1],(YoshiInsets){0},7.0/6.0,&g));
    assert(g.logical_width > 256 && g.logical_height == 224);
    assert(fabs(g.original_width / (224*g.scale) - 4.0/3.0) < 1e-9);
    assert(g.logical_width*g.scale*g.pixel_aspect >= g.width);
    assert((g.logical_width-1)*g.scale*g.pixel_aspect < g.width);
    printf("%dx%d -> logical %dx%d\n",g.width,g.height,g.logical_width,g.logical_height);
  }
  YoshiDisplayGeometry g;
  assert(yoshi_display_geometry(2772,1272,(YoshiInsets){80,0,20,30},1,&g));
  assert(g.x==80 && g.width==2672 && g.height==1242);
  assert(!yoshi_display_geometry(10,10,(YoshiInsets){10,0,0,0},1,&g));
  assert(!yoshi_display_geometry(10,10,(YoshiInsets){0},NAN,&g));
  assert(!yoshi_display_geometry(10,10,(YoshiInsets){-1,0,0,0},1,&g));
  return 0;
}
