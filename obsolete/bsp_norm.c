#include <math.h>
#include "bsp_norm.h"
#include "mm.h"
#include <SDL/SDL_opengl.h>

typedef struct {
  double x, y, z;
  double nx, ny, nz;
  int n;
} normal_t;

static struct {
  normal_t *p;
  unsigned alloc, n;
} nc;

void bspPutNormal(double x, double y, double z, double nx, double ny, double nz) {
  int i;
  normal_t *n;
  for (i = nc.n, n = nc.p; i; --i, ++n)
    if (n->x == x && n->y == y && n->z == z) {
      n->nx += nx;
      n->ny += ny;
      n->nz += nz;
      ++n->n;
      return;
    }
  if (nc.n == nc.alloc) {
    nc.alloc *= 2;
    normal_t *p = (normal_t *)mmAlloc(nc.alloc * sizeof(normal_t));
    if (p == NULL) return;
    for (i = 0; i < nc.n; ++i) p[i] = nc.p[i];
    mmFree(nc.p);
    nc.p = p;
  }
  n = nc.p + nc.n++;
  n->x = x;
  n->y = y;
  n->z = z;
  n->nx = nx;
  n->ny = ny;
  n->nz = nz;
  n->n = 1;
}

double *bspGetNormal(double x, double y, double z) {
  static double failsafe[3] = { 0.0, 0.0, 0.0 };
  int i;
  normal_t *n;
  for (i = nc.n, n = nc.p; i; --i, ++n)
    if (n->x == x && n->y == y && n->z == z) {
      if (n->n > 1) {
        n->nx /= n->n;
        n->ny /= n->n;
        n->nz /= n->n;
        n->n = 1;
        double l = 1 / sqrt(n->nx * n->nx + n->ny * n->ny/* + n->nz * n->nz*/);
        n->nx *= l;
        n->ny *= l;
        n->nz *= l;
      }
      return &n->nx;
    }
  return failsafe;
}

void bspFlushNormals() {
  nc.n = 0;
}

int bspInitNorm() {
  nc.alloc = 8;
  nc.p = (normal_t *)mmAlloc(nc.alloc * sizeof(normal_t));
  if (nc.p == NULL) return 0;
  nc.n = 0;
  return !0;
}

void bspDoneNorm() {
  if (nc.p != NULL) {
    mmFree(nc.p);
    nc.p = NULL;
  }
  nc.alloc = nc.n = 0;
}

void bspDrawNormals(double x, double y, double z) {
  int i;
  normal_t *n;
  glBegin(GL_LINES);
  glColor3ub(0xff, 0xff, 0xff);
  for (i = nc.n, n = nc.p; i; --i, ++n) {
    glVertex3d(n->x - x, n->y - y, n->z - z);
    glVertex3d(n->x - x + 10*n->nx, n->y - y + 10*n->ny, n->z - z + 10*n->nz);
  }
  glEnd();
}
