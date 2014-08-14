#ifndef _BSP_H
#define _BSP_H 1

#include "bbox.h"

#include <cstdio>

typedef struct {
  float x, y, z;
} vec3d_t;

typedef struct {
  float x, y;
} vertex_t;

typedef struct {
  short f;
  short c;
  unsigned char l;
  short u;
  short v;
  unsigned t;
} sector_t;

struct node_s;

typedef enum {
  LF_NOTHING		= 0x0000,
  LF_TWOSIDED		= 0x0001,
  LF_TOPSTART		= 0x0002,
  LF_BOTTOMSTART	= 0x0004,
  LF_BLOCK		= 0x0008
} lineflag_t;

typedef struct {
  unsigned a, b;
  float u1, u2, v;
  lineflag_t flags;
  float nx, ny;
  unsigned t;
  int backSectorId;
} line_t;

typedef enum {
  NF_NOTHING	= 0x00,
  NF_VISIBLE	= 0x01
} nodeflag_t;

class Plane2d {
public:
  Plane2d() : a_(), b_(), c_() {}
  Plane2d(const vertex_t& v1, const vertex_t& v2)
  : a_(v2.y - v1.y)
  , b_(v1.x - v2.x)
  , c_(-(a_ * v1.x + b_ * v1.y))
  {}

  double determine(const vertex_t& v) const { return a_ * v.x + b_ * v.y + c_; }
  double dot(double dx, double dy) const { return a_ * dy - b_ * dx; }
  void load(FILE* fp);
  void print() const;

private:
  double a_, b_, c_;
};

typedef struct node_s {
  line_t *p;
  unsigned n;
  bbox_t bb;
  sector_t *s;
  struct node_s *l, *r, *ow;
  nodeflag_t flags;
  Plane2d div;
} node_t;

typedef struct {
  vertex_t *p;
  unsigned n;
} vc_t;

typedef struct {
  sector_t *p;
  unsigned n;
} sc_t;

extern node_t *root, *cn;
extern vc_t vc;
extern sc_t sc;

void bspCollideTree(float x, float y, float z, float *dx, float *dy, float *dz, int hard);
node_t *bspGetNodeForCoords(float x, float y, float z);
void bspFreeMap();
int bspLoadMap(const char *fname);
void bspInit();
void bspDone();
int bspIsLoaded();

#endif /* _BSP_H */
