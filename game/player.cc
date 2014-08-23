/* vim: set ts=2 sw=8 tw=0 et :*/
#include <string.h>
#include <math.h>
#include <SDL/SDL.h>
#include "player.h"
#include "render.h"
#include "console.h"
#include "bsp.h"
#include "cmd.h"

Player cam;

static void plRotateCam(float a, float b) {
  float beta;
  float fov2;

  a += cam.a;
  while (a >= 2 * M_PI) a -= 2 * M_PI;
  while (a < 0.0) a += 2 * M_PI;
  b += cam.b;
  if (b > M_PI / 2) b = M_PI / 2;
  if (b < -M_PI / 2) b = -M_PI / 2;
  cam.a = a;
  cam.b = b;
  cam.forward = Vec3d(sin(a) * cos(b), cos(a) * cos(b), -sin(b));
  cam.right = Vec3d(cos(a), -sin(a), 0.0f);
  cam.up = cross(cam.right, cam.forward);
  fov2 = curfov / 360.0 * M_PI;
  beta = b - fov2;
  cam.v[0] = Vec3d(cos(beta) * sin(a) + sin(-fov2) * cos(a), cos(beta) * cos(a) - sin(-fov2) * sin(a), -sin(beta));
  cam.v[1] = Vec3d(cos(beta) * sin(a) + sin(fov2) * cos(a), cos(beta) * cos(a) - sin(fov2) * sin(a), -sin(beta));
  beta = b + fov2;
  cam.v[2] = Vec3d(cos(beta) * sin(a) + sin(fov2) * cos(a), cos(beta) * cos(a) - sin(fov2) * sin(a), -sin(beta));
  cam.v[3] = Vec3d(cos(beta) * sin(a) + sin(-fov2) * cos(a), cos(beta) * cos(a) - sin(-fov2) * sin(a), -sin(beta));

  for (int i = 0; i < 4; ++i) cam.cps[i] = cross(cam.v[i], cam.v[(i + 1) % 4]);
}

#define CAM_DA_MIN (M_PI/3000)
#define CAM_DA_MAX (M_PI/100)
#define CAM_DA_INC (M_PI/1200)
#define CAM_DA_DEC (M_PI/2000)
#define CAM_V_MIN 0.001
#define CAM_V_MAX 3.0
#define CAM_V_INC 0.2
#define CAM_V_DEC 0.05
#define FALL_V_MAX 10.0
#define FALL_V_INC 0.2
#define FOVINC 10.0
#define FOVDEC 20.0

static int clip = 1;
static int gravity = 1;
static float fov = 90.0;
static float zoomfov = 25.0;
static float mouse_sensitivity = 0.004;
static int onfloor = 0;

static int cmd_move(int argc, char **argv) {
  if (argc < 2) {
    cmsg(MLINFO, "usage: %s <forward|back|left|right|up|down>", *argv);
    return !0;
  }
  if (conGetState() != CON_INACTIVE) return 0;
  if (!strcmp(argv[1], "forward")) {
    cam.sumForces += cam.forward;
  } else if (!strcmp(argv[1], "back")) {
    cam.sumForces -= cam.forward;
  } else if (!strcmp(argv[1], "left")) {
    cam.sumForces -= cam.right;
  } else if (!strcmp(argv[1], "right")) {
    cam.sumForces += cam.right;
  } else if (!strcmp(argv[1], "up")) {
//    cam.sumForces += cam.up;
    if (onfloor) cam.force(Vec3d(0.0f, 0.0f, 3.0f), 1.0f);
  } else if (!strcmp(argv[1], "down")) {
    cam.sumForces -= cam.up;
  } else {
    cmsg(MLERR, "cmd_move: unknown direction: %s", argv[1]);
    return !0;
  }
  return 0;
}

static int cmd_turn(int argc, char **argv) {
  if (argc < 2) {
    cmsg(MLINFO, "usage: %s <left|right>", *argv);
    return !0;
  }
  if (conGetState() != CON_INACTIVE) return 0;
  if (!strcmp(argv[1], "left") && cam.da > -CAM_DA_MAX)
    cam.da -= CAM_DA_INC;
  else if (!strcmp(argv[1], "right") && cam.da < CAM_DA_MAX)
    cam.da += CAM_DA_INC;
  return 0;
}

static int cmd_zoom(int argc, char **argv) {
  (void)argc;
  (void)argv;
  if (conGetState() != CON_INACTIVE) return 0;
  curfov -= FOVDEC;
  return 0;
}

void plUpdate() {
  int mx, my;

  SDL_GetRelativeMouseState(&mx, &my);
  if (gravity && clip) cam.force(Vec3d(0.0f, 0.0f, -FALL_V_INC), 1.0f);
  if (len(cam.sumForces) > std::numeric_limits<double>::epsilon()) {
    cam.force(0.2f * norm(cam.sumForces), 1.0f);
  }
  cam.friction(onfloor ? 0.1f : 0.075f);
  onfloor = 0;
  if (clip) {
    double vx = cam.velo().x();
    double vy = cam.velo().y();
    double vz = cam.velo().z();
    bspCollideTree(cam.pos(), &vx, &vy, &vz, 0);
    bspCollideTree(cam.pos(), &vx, &vy, &vz, 1);
    if (cam.velo().z() < 0.0f && vz > cam.velo().z()) onfloor = !0;
    cam.velo(Vec3d(vx, vy, vz));
  }
  cam.move(1.0f);
  cam.a += cam.da;
  curfov += FOVINC;
  if (curfov > fov)
    curfov = fov;
  else if (curfov < zoomfov)
    curfov = zoomfov;
  if (cam.da > CAM_DA_MIN)
    cam.da -= CAM_DA_DEC;
  else if (cam.da < -CAM_DA_MIN)
    cam.da += CAM_DA_DEC;
  else
    cam.da = 0;
  plRotateCam(mouse_sensitivity * mx, mouse_sensitivity * my);
  cn = bspGetNodeForCoords(cam.pos());
  cam.sumForces = Vec3d();
}

static void rFovSet(void *addr) {
  float *d = reinterpret_cast<float*>(addr);
  if (*d > 120.0) {
    cmsg(MLWARN, "maximal value is 120.0, clamped");
    *d = 120.0;
  }
  if (*d < 10.0) {
    cmsg(MLWARN, "maximal value is 10.0, clamped");
    *d = 10.0;
  }
  plRotateCam(0.0, 0.0);
}

void plSetPosition(float x, float y, float z, float a) {
  cam.pos(Vec3d(x, y, z));
  cam.stop();
  cam.a = a;
  cam.da = 0.0;
  cam.b = 0.0;
  plRotateCam(0.0, 0.0);
}

void plInit() {
  plSetPosition(0.0, 0.0, 0.0, 0.0);
  cmdAddBool("gravity", &gravity, 1);
  cmdAddBool("clip", &clip, 1);
  cmdAddFloat("zoomfov", &zoomfov, 1);
  cmdSetAccessFuncs("zoomfov", NULL, rFovSet);
  cmdAddFloat("fov", &fov, 1);
  cmdSetAccessFuncs("fov", NULL, rFovSet);
  cmdAddFloat("mouse_sensitivity", &mouse_sensitivity, 1);
  cmdAddCommand("move", cmd_move);
  cmdAddCommand("turn", cmd_turn);
  cmdAddCommand("zoom", cmd_zoom);
  cmdAddCommand("attack", cmd_null);
}

void plDone() {
}
