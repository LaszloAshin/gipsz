#include "bbox.h"
#include "player.h"

void bbAdd(bbox_t *bb, int x, int y, int z) {
  if (x < bb->x1) bb->x1 = x;
  if (x > bb->x2) bb->x2 = x;
  if (y < bb->y1) bb->y1 = y;
  if (y > bb->y2) bb->y2 = y;
  if (z < bb->z1) bb->z1 = z;
  if (z > bb->z2) bb->z2 = z;
}

int bbVisible(bbox_t *bb) {
  vec3d_t p[8];
  p[0].x = p[3].x = p[4].x = p[7].x = bb->x1 - cam.x;
  p[1].x = p[2].x = p[5].x = p[6].x = bb->x2 - cam.x;
  p[0].y = p[1].y = p[4].y = p[5].y = bb->y1 - cam.y;
  p[2].y = p[3].y = p[6].y = p[7].y = bb->y2 - cam.y;
  p[4].z = p[5].z = p[6].z = p[7].z = bb->z1 - cam.z;
  p[0].z = p[1].z = p[2].z = p[3].z = bb->z2 - cam.z;

  int i, j, k;
  double x, y, z;
  for (i = 0; i < 5; ++i) {
    switch (i) {
      case 4:
        x = -cam.dx;
        y = -cam.dy;
        z = -cam.dz;
        break;
      default:
        x = -cam.cps[i].x;
        y = -cam.cps[i].y;
        z = -cam.cps[i].z;
        break;
    }
    k = 8;
    for (j = 0; j < 8; ++j)
      if (p[j].x * x + p[j].y * y + p[j].z * z > 0) --k;
    if (!k) return 0;
  }
  return !0;
}

int bbInside(bbox_t *bb, double x, double y, double z, double b) {
  return (
    x >= bb->x1 - b && x <= bb->x2 + b &&
    y >= bb->y1 - b && y <= bb->y2 + b &&
    z >= bb->z1 - b && z <= bb->z2 + b
  );
}
