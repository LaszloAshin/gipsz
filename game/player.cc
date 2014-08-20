#include <string.h>
#include <math.h>
#include <SDL/SDL.h>
#include "player.h"
#include "render.h"
#include "console.h"
#include "bsp.h"
#include "cmd.h"

cam_t cam;

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
  cam.d2x = sin(a);
  cam.d2y = cos(a);
  cam.dx = cam.d2x * cos(b);
  cam.dy = cam.d2y * cos(b);
  cam.dz = -sin(b);
  fov2 = curfov / 360.0 * M_PI;
  beta = b - fov2;
  cam.v[0] = Vec3d(cos(beta) * sin(a) + sin(-fov2) * cos(a), cos(beta) * cos(a) - sin(-fov2) * sin(a), -sin(beta));
  cam.v[1] = Vec3d(cos(beta) * sin(a) + sin(fov2) * cos(a), cos(beta) * cos(a) - sin(fov2) * sin(a), -sin(beta));
  beta = b + fov2;
  cam.v[2] = Vec3d(cos(beta) * sin(a) + sin(fov2) * cos(a), cos(beta) * cos(a) - sin(fov2) * sin(a), -sin(beta));
  cam.v[3] = Vec3d(cos(beta) * sin(a) + sin(-fov2) * cos(a), cos(beta) * cos(a) - sin(-fov2) * sin(a), -sin(beta));

  for (int i = 0; i < 4; ++i) cam.cps[i] = cam.v[i] % cam.v[(i + 1) % 4];
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
  float x, y, z;

  if (argc < 2) {
    cmsg(MLINFO, "usage: %s <forward|back|left|right|up|down>", *argv);
    return !0;
  }
  if (conGetState() != CON_INACTIVE) return 0;
  if (!strcmp(argv[1], "forward")) {
    if (gravity) {
      cam.vx += cam.d2x * CAM_V_INC;
      cam.vy += cam.d2y * CAM_V_INC;
    } else {
      cam.vx += cam.dx * CAM_V_INC;
      cam.vy += cam.dy * CAM_V_INC;
      cam.vz += cam.dz * CAM_V_INC;
    }
  } else if (!strcmp(argv[1], "back")) {
    if (gravity) {
      cam.vx -= cam.d2x * CAM_V_INC;
      cam.vy -= cam.d2y * CAM_V_INC;
    } else {
      cam.vx -= cam.dx * CAM_V_INC;
      cam.vy -= cam.dy * CAM_V_INC;
      cam.vz -= cam.dz * CAM_V_INC;
    }
  } else if (!strcmp(argv[1], "left")) {
    cam.vx -= cam.d2y * CAM_V_INC;
    cam.vy += cam.d2x * CAM_V_INC;
  } else if (!strcmp(argv[1], "right")) {
    cam.vx += cam.d2y * CAM_V_INC;
    cam.vy -= cam.d2x * CAM_V_INC;
  } else if (!strcmp(argv[1], "up")) {
    if (onfloor) cam.vz = 4.0;
    if (!gravity) {
      x = -cam.d2x * cam.dz;
      y = -cam.d2y * cam.dz;
      z = cam.d2y * cam.dy + cam.d2x * cam.dx;
      cam.vx += x * CAM_V_INC;
      cam.vy += y * CAM_V_INC;
      cam.vz += z * CAM_V_INC;
    }
  } else if (!strcmp(argv[1], "down")) {
    if (!gravity) {
      x = -cam.d2x * cam.dz;
      y = -cam.d2y * cam.dz;
      z = cam.d2y * cam.dy + cam.d2x * cam.dx;
      cam.vx -= x * CAM_V_INC;
      cam.vy -= y * CAM_V_INC;
      cam.vz -= z * CAM_V_INC;
    }
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
  float l;
  float vz;

  SDL_GetRelativeMouseState(&mx, &my);
  if (gravity) {
    if (clip) cam.vz -= FALL_V_INC;
    l = sqrt(cam.vx * cam.vx + cam.vy * cam.vy);
    if (l > CAM_V_MAX) {
      l = 1 / l;
      cam.vx *= l;
      cam.vy *= l;
      l = CAM_V_MAX;
    }
  } else {
    l = sqrt(cam.vx * cam.vx + cam.vy * cam.vy + cam.vz * cam.vz);
    if (l > CAM_V_MAX) {
      l = 1 / l;
      cam.vx *= l;
      cam.vy *= l;
      cam.vz *= l;
      l = CAM_V_MAX;
    }
  }
  if (cam.vz < -FALL_V_MAX) cam.vz = -FALL_V_MAX;
  if (cam.vz > FALL_V_MAX) cam.vz = FALL_V_MAX;
  onfloor = 0;
  if (clip) {
    vz = cam.vz;
    bspCollideTree(cam.x, cam.y, cam.z, &cam.vx, &cam.vy, &cam.vz, 0);
    bspCollideTree(cam.x, cam.y, cam.z, &cam.vx, &cam.vy, &cam.vz, 1);
    if (vz < 0.0 && cam.vz > vz) ++onfloor;
  }
  cam.x += cam.vx;
  cam.y += cam.vy;
  cam.z += cam.vz;
  cam.a += cam.da;
  curfov += FOVINC;
  if (curfov > fov)
    curfov = fov;
  else if (curfov < zoomfov)
    curfov = zoomfov;
  if (l > CAM_V_MIN) {
    cam.vx -= CAM_V_DEC * cam.vx;
    cam.vy -= CAM_V_DEC * cam.vy;
    if (!gravity) cam.vz -= CAM_V_DEC * cam.vz;
  } else {
    cam.vx = cam.vy = 0.0;
    if (!gravity) cam.vz = 0.0;
  }
  if (cam.da > CAM_DA_MIN)
    cam.da -= CAM_DA_DEC;
  else if (cam.da < -CAM_DA_MIN)
    cam.da += CAM_DA_DEC;
  else
    cam.da = 0;
  plRotateCam(mouse_sensitivity * mx, mouse_sensitivity * my);
  cn = bspGetNodeForCoords(cam.x, cam.y, cam.z);
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
  cam.x = x;
  cam.y = y;
  cam.z = z;
  cam.a = a;
  cam.vx = cam.vy = cam.vz = 0.0;
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
