#ifndef _PLAYER_H
#define _PLAYER_H 1

#include <lib/vec.hh>
#include "bsp.h"

typedef struct {
  float x, y, z;
  float dx, dy, dz;
  float d2x, d2y;
  float vx, vy, vz;
  float a, da;
  float b;
  Vec3d cps[4];
  Vec3d v[4];
} cam_t;

extern cam_t cam;

void plSetPosition(float x, float y, float z, float a);
void plUpdate();
void plInit();
void plDone();

#endif /* _PLAYER_H */
