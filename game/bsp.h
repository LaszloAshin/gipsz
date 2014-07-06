#ifndef _BSP_H
#define _BSP_H 1

#include "bbox.h"

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
  LF_BLOCK		= 0x0008,
} lineflag_t;

typedef struct {
  unsigned a, b;
  float u1, u2, v;
  struct node_s *nn;
  lineflag_t flags;
  float nx, ny;
  unsigned t;
} line_t;

typedef enum {
  NF_NOTHING	= 0x00,
  NF_VISIBLE	= 0x01,
} nodeflag_t;

typedef struct node_s {
  line_t *p;
  unsigned n;
  bbox_t bb;
  sector_t *s;
  struct node_s *l, *r, *ow;
  nodeflag_t flags;
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
