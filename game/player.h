#ifndef _PLAYER_H
#define _PLAYER_H 1

#include <lib/vec.hh>
#include <lib/masspoint.hh>
#include "bsp.h"

struct Player : public MassPoint3d {
  Vec3d forward;
  Vec3d right;
  Vec3d up;
  Vec3d sumForces;
  float a, da; // yaw axis
  float b; // pitch axis
  // roll axis not used
  Vec3d cps[4];
  Vec3d v[4];
};

extern Player cam;

void plSetPosition(float x, float y, float z, float a);
void plUpdate();
void plInit();
void plDone();

#endif /* _PLAYER_H */
