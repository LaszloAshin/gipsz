#include <stdlib.h>
#include <math.h>
#include <SDL/SDL_opengl.h>
#include "spring.h"
#include "main.h"
#include "input.h"
#include "console.h"
#include "cmd.h"

static int springEnabled = 0;

static drawFunc_t oldDraw = NULL;
static updateFunc_t oldUpdate = NULL;
static keyFunc_t oldKey = NULL;
static mouseFunc_t oldMouse = NULL;

typedef struct {
  double x, y, z;
  double vx, vy, vz;
  double Fx, Fy, Fz;
  int fixed;
} vertex_t;

typedef struct {
  int a, b;
  double l;
} spring_t;

#define NV 125
#define NS 1280
//144
#define PI 3.141592654

static vertex_t vc[NV];
static spring_t sc[NS];
static double va = 0, vb = 0;
static double nrg = 0.0;
static double mot = 0.1;
static int cntr;

static double dabs(double d) { return (d < 0) ? -d : d; }

static double springLen(int i) {
  double x, y, z;
  x = vc[sc[i].b].x - vc[sc[i].a].x;  x *= x;
  y = vc[sc[i].b].y - vc[sc[i].a].y;  y *= y;
  z = vc[sc[i].b].z - vc[sc[i].a].z;  z *= z;
  return sqrt(x + y + z);
}

static void springRand() {
  int i;
  for (i = 0; i < NV; ++i) {
    if (vc[i].fixed) continue;
    vc[i].x = (rand() % 50) - 25;
    vc[i].y = (rand() % 50) - 25;
    vc[i].z = (rand() % 50) - 25;
    vc[i].vx = vc[i].vy = vc[i].vz = 0.0;
  }
  mot = 0.1;
  cntr = 0;
/*  int j, n;
  for (i = 0; i < 4; ++i)
    for (j = 0; j < 4; ++j)
      for (n = 0; n < 4; ++n) {
        if (!vc[n*16+j*4+i].fixed) continue;
        vc[n*16+j*4+i].x = ((double)(i) - 1.5) * 10.1;
        vc[n*16+j*4+i].y = ((double)(j) - 1.5) * 10.1;
        vc[n*16+j*4+i].z = ((double)(n) - 1.5) * 10.1;
      }*/
}

static void springDraw(float mx, float my) {
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 10000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslated(0.0, 0.0, -50.0);
  glRotated(vb / PI * 180.0, 1.0, 0.0, 0.0);
  glRotated(va / PI * 180.0, 0.0, 0.0, 1.0);
  glColor3f(1.0, 1.0, 1.0);
  glBegin(GL_LINES);
  int i;
  for (i = 0; i < NS; ++i) {
    glVertex3f(vc[sc[i].a].x, vc[sc[i].a].y, vc[sc[i].a].z);
    glVertex3f(vc[sc[i].b].x, vc[sc[i].b].y, vc[sc[i].b].z);
  }
  glEnd();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  dmsg(MLDBG, "nrg %.20lf", nrg);
  dmsg(MLDBG, "mot %.20lf", mot);
  dmsg(MLDBG, "cnt %d", cntr);
}

static void springUpdate() {
  int mx, my;
  SDL_GetRelativeMouseState(&mx, &my);
  va += 0.004 * mx;
  vb += 0.004 * my;
  while (va >= 2 * PI) va -= 2 * PI;
  while (va < 0.0) va += 2 * PI;
  while (vb >= 2 * PI) vb -= 2 * PI;
  while (vb < 0.0) vb += 2 * PI;
/*  if (vb > PI / 2) vb = PI / 2;
  if (vb < -PI / 2) vb = -PI / 2;*/
  int i;
  /* init forces */
  for (i = 0; i < NV; ++i)
    vc[i].Fx = vc[i].Fy = vc[i].Fz = 0.0;
  /* compute forces and energy on each springs */
  double pe = nrg;
  double x, y, z;
  nrg = 0.0;
  for (i = 0; i < NS; ++i) {
    x = vc[sc[i].b].x - vc[sc[i].a].x;
    y = vc[sc[i].b].y - vc[sc[i].a].y;
    z = vc[sc[i].b].z - vc[sc[i].a].z;
    double n = sqrt(x * x + y * y + z * z);
    if (dabs(n) < 0.0001) continue;
    double d = n - sc[i].l;
    nrg += d * d;
    n = d / n;
    x *= n, y *= n, z *= n;
    vc[sc[i].a].Fx += x;
    vc[sc[i].a].Fy += y;
    vc[sc[i].a].Fz += z;
    vc[sc[i].b].Fx -= x;
    vc[sc[i].b].Fy -= y;
    vc[sc[i].b].Fz -= z;
  }
  /* move nodes */
  mot *= (nrg >= pe) ? 0.90 : 1.01;
  for (i = 0; i < NV; ++i) {
    if (vc[i].fixed) continue;
    vc[i].vx += vc[i].Fx * mot;
    vc[i].vy += vc[i].Fy * mot;
    vc[i].vz += vc[i].Fz * mot;
    vc[i].x += vc[i].vx;
    vc[i].y += vc[i].vy;
    vc[i].z += vc[i].vz;
    vc[i].vx *= 0.8;
    vc[i].vy *= 0.8;
    vc[i].vz *= 0.8;
  }
  ++cntr;
}

static void springKey(SDL_KeyboardEvent *ev) {
  switch (ev->keysym.sym) {
    case SDLK_ESCAPE:
      springDisable();
      break;
    default: break;
  }
}

void springInit() {
  int i, j, ns, n, min, minn = 0;
  springRand();
  for (i = 0; i < 5; ++i)
    for (j = 0; j < 5; ++j)
      for (n = 0; n < 5; ++n) {
        vc[n*25+j*5+i].x = ((double)(i) - 2.0) * 10.0;
        vc[n*25+j*5+i].y = ((double)(j) - 2.0) * 10.0;
        vc[n*25+j*5+i].z = ((double)(n) - 2.0) * 10.0;
        vc[n*25+j*5+i].fixed = 0;
      }
/*  vc[0].fixed = 1;
  vc[3].fixed = 1;
  vc[12].fixed = 1;
  vc[15].fixed = 1;
  vc[48].fixed = 1;
  vc[51].fixed = 1;
  vc[60].fixed = 1;
  vc[63].fixed = 1;*/
/*  for (ns = 0; ns < NS; ++ns) {
    min = -1;
    for (i = 0; i < NV; ++i) {
      n = 0;
      for (j = 0; j < ns; ++j)
        if (sc[j].a == i || sc[j].b == i) ++n;
      if (min < 0 || n < minn) {
        min = i;
        minn = n;
      }
    }
    if (min < 0) break;
    int a = min;
    min = -1;
    for (i = 0; i < NV; ++i) {
      if (i == a) continue;
      n = 0;
      for (j = 0; j < ns; ++j)
        if (sc[j].a == i || sc[j].b == i) ++n;
      if (min < 0 || n < minn) {
        min = i;
        minn = n;
      }
    }
    if (min < 0) break;
    sc[ns].a = a;
    sc[ns].b = min;
  }*/
  for (ns = 0; ns < NS;) {
    minn = ns;
    for (i = 0; i < NV; ++i)
      for (j = 0; j < NV; ++j) {
        if (j == i) continue;
        if (ns >= NS) continue;
        min = 0;
        for (n = 0; n < ns; ++n)
          if ((sc[n].a == i && sc[n].b == j) || (sc[n].a == j && sc[n].b == i)) {
            ++min;
            break;
          }
        if (min) continue;
/*        if ((vc[i].x != vc[j].x && vc[i].y != vc[j].y) || dabs(vc[i].z - vc[j].z) > 10.0) continue;
        if ((vc[i].x != vc[j].x && vc[i].z != vc[j].z) || dabs(vc[i].y - vc[j].y) > 10.0) continue;
        if ((vc[i].z != vc[j].z && vc[i].y != vc[j].y) || dabs(vc[i].x - vc[j].x) > 10.0) continue;*/
        sc[ns].a = i;
        sc[ns].b = j;
        sc[ns].l = springLen(ns);
        if (sc[ns].l > 20.0) continue;
        ++ns;
      }
    if (minn == ns) break;
  }
//  for (i = 0; i < NS; ++i) sc[i].l = springLen(i);
}

static int cmd_attack(int argc, char **argv) {
  springRand();
  return 0;
}

void springEnable() {
  if (springEnabled) return;

  springInit();
  mouseHide();

  oldDraw = drawFunc;
  oldUpdate = updateFunc;
  oldKey = keyFunc;
  oldMouse = mouseFunc;

  drawFunc = springDraw;
  updateFunc = springUpdate;
  keyFunc = springKey;
  mouseFunc = NULL;

  cmdAddCommand("attack", cmd_attack);

  ++springEnabled;
}

void springDisable() {
  if (!springEnabled) return;

  drawFunc = oldDraw;
  updateFunc = oldUpdate;
  keyFunc = oldKey;
  mouseFunc = oldMouse;

  mouseShow();

  springEnabled = 0;
}
