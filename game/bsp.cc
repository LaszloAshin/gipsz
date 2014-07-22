#include <stdlib.h>
#include <math.h>
#include "bsp.h"
#include "player.h"
#include "mm.h"
#include "console.h"
#include "tex.h"
#include "render.h"
#include "cmd.h"
#include "game.h"
#include "menu.h"
#include "model.h"
#include "obj.h"

void
Plane2d::load(FILE* fp)
{
  fread(&a_, sizeof(a_), 1, fp);
  fread(&b_, sizeof(b_), 1, fp);
  fread(&c_, sizeof(c_), 1, fp);
}

vc_t vc;
sc_t sc;

node_t *root = NULL, *cn = NULL;

static int bspLoaded = 0;
int bspIsLoaded() { return bspLoaded; }

/* point-line distance */
static float pszt(vertex_t p1, vertex_t p2, vertex_t p3, vertex_t *p4) {
  float dx21, dy21;
  float x, y;

  if (p1.x > p2.x) {
    vertex_t p = p1;
    p1 = p2;
    p2 = p;
  }
  dx21 = p2.x - p1.x;
  dy21 = p2.y - p1.y;
  x = p3.y - p1.y;
  x *= dx21 * dy21;
  x += p1.x * dy21 * dy21 + p3.x * dx21 * dx21;
  x /= dy21 * dy21 + dx21 * dx21;
  if (x < p1.x) {
    x = p1.x;
    y = p1.y;
  } else if (x > p2.x) {
    x = p2.x;
    y = p2.y;
  } else {
    if (fabs(dx21) > fabs(dy21)) {
      if (dx21)
        y = p1.y + (x - p1.x) * dy21 / dx21;
      else
        return 0;
    } else if (dy21)
      y = p3.y - (x - p3.x) * dx21 / dy21;
    else
      return 0;
    if (p1.y > p2.y) {
      vertex_t p = p1;
      p1 = p2;
      p2 = p;
    }
    if (y < p1.y) {
      y = p1.y;
      x = p1.x;
    } else if (y > p2.y) {
      y = p2.y;
      x = p2.x;
    }
  }
  dx21 = p3.x - x;
  dy21 = p3.y - y;
  if (p4 != NULL) {
    p4->x = dx21;
    p4->y = dy21;
  }
  return dx21 * dx21 + dy21 * dy21;
}

/* arc tan function which computes angle from dx and dy */
float at(float dy, float dx) {
  if (dx > 0)
    return atanf(dy / dx);
  else if (dx < 0)
    return ((dy > 0) ? M_PI : -M_PI) - atanf(dy / -dx);
  else
    return (dy > 0) ? M_PI/2 : -M_PI/2;
}

struct collide_param_rec {
  float x, y, z;
  float *dx, *dy, *dz;
  int hard;
  int j;
};

static void
bspCollideNode(struct collide_param_rec *pr, node_t *n)
{
  if (n == NULL || !bbInside(&n->bb, pr->x, pr->y, pr->z, 48.0)) return;
  bspCollideNode(pr, n->l);
  bspCollideNode(pr, n->r);
  if (n->s == NULL || !n->n) return;
  vertex_t p;
  p.x = pr->x + *pr->dx;
  p.y = pr->y + *pr->dy;
  float pz = pr->z + *pr->dz;
  int in = 1;
  for (line_t *l = n->p; l < n->p + n->n; ++l) {
    float dx1 = vc.p[l->b].x - vc.p[l->a].x;
    const float dy1 = vc.p[l->b].y - vc.p[l->a].y;
    float dx2 = vc.p[l->b].x - pr->x;
    const float dy2 = vc.p[l->b].y - pr->y;
    const int front = dx1 * dy2 < dy1 * dx2;
    if (!front) in = 0;
    if (n->s->f < n->s->c) {
      if (pz < n->s->f - 16 - 8 || pz > n->s->c + 48 + 8) continue;
    } else {
      if (pz < n->s->c - 16 - 8 || pz > n->s->f + 48 + 8) continue;
    }
    vertex_t f = { 0, 0 };
    if (pszt(vc.p[l->a], vc.p[l->b], p, &f) > 256.0) continue;
    in = 0;
    if (pz > n->s->f && pz < n->s->f + 48) {
      *pr->dz = (n->s->f + 48 - pr->z) * 0.1;
      pz = pr->z + *pr->dz;
    }
    if (pz < n->s->c && pz > n->s->c - 16) {
      *pr->dz = (n->s->c - 16 - pr->z) * 0.1;
      pz = pr->z + *pr->dz;
    }
    const sector_t *ns = (l->nn != NULL) ? l->nn->s : NULL;
    if (ns != NULL) {
      if (n->s->f < n->s->c) {
        dx1 = n->s->f;
        if (ns->f > dx1) dx1 = ns->f;
        dx2 = n->s->c;
        if (ns->c < dx2) dx2 = ns->c;
        if (dx2 - dx1 > 64 && ns->f < pz - 48 + 24 && ns->c > pz + 16) continue;
      } else {
        if (ns->f <= pz - 48 + 24 || ns->c >= pz + 16) continue;
      }
    } else if (n->s->f > n->s->c) {
      if (pz > n->s->f + 48 - 8 || pz < n->s->c - 16 + 8) continue;
    }
    const float q = sqrtf(f.x * f.x + f.y * f.y);
    f.x /= q;  f.y /= q;
    if (pr->hard) {
      if (pr->j) {
        *pr->dx += f.x;
        *pr->dy += f.y;
      } else {
        *pr->dx = f.x;
        *pr->dy = f.y;
      }
      ++pr->j;
      continue;
    }
    const float q2 = cosf(at(f.x, -f.y) - at(*pr->dy, *pr->dx)) * sqrtf(*pr->dx * *pr->dx + *pr->dy * *pr->dy);
    *pr->dx = -f.y * q2;
    *pr->dy = f.x * q2;
  }
  if (in) {
    if (pz > n->s->f && pz < n->s->f + 48) {
      *pr->dz = (n->s->f + 48 - pr->z) * 0.1;
      pz = pr->z + *pr->dz;
    }
    if (pz < n->s->c && pz > n->s->c - 16) {
      *pr->dz = (n->s->c - 16 - pr->z) * 0.1;
      pz = pr->z + *pr->dz;
    }
  }
}

/* function to handle point-bsptree collision */
void bspCollideTree(float x, float y, float z, float *dx, float *dy, float *dz, int hard) {
  struct collide_param_rec pr;
  pr.x = x;
  pr.y = y;
  pr.z = z;
  pr.dx = dx;
  pr.dy = dy;
  pr.dz = dz;
  pr.hard = hard;
  pr.j = 0;
  if (root != NULL) bspCollideNode(&pr, root);
}

static node_t *
bspGetNodeForCoordsSub(node_t *n, float x, float y, float z)
{
  if (n->l == NULL && n->r == NULL && n->n && n->s->f < n->s->c && bbInside(&n->bb, x, y, z, 0.0)) {
    int bo = 0;
    vertex_t *b = vc.p + n->p[0].a;
    for (line_t *l = n->p; l < n->p + n->n; ++l) {
      vertex_t *a = b;
      b = vc.p + l->b;
      const float dx1 = b->x - a->x;
      const float dy1 = b->y - a->y;
      const float dx2 = b->x - x;
      const float dy2 = b->y - y;
      if (dx1 * dy2 > dy1 * dx2) {
        ++bo;
        break;
      }
    }
    if (!bo) {
      return n;
    }
  }
  if (n->l != NULL) {
    node_t *result = bspGetNodeForCoordsSub(n->l, x, y, z);
    if (result) return result;
  }
  if (n->r != NULL) {
    return bspGetNodeForCoordsSub(n->r, x, y, z);
  }
  return 0;
}

node_t *bspGetNodeForCoords(float x, float y, float z) {
  return root ? bspGetNodeForCoordsSub(root, x, y, z) : 0;
}

static node_t *
bspGetNodeForLineSub(node_t *n, unsigned a, unsigned b)
{
  for (line_t *l = n->p; l < n->p + n->n; ++l) {
    if ((l->flags & LF_TWOSIDED) && l->a == a && l->b == b) {
      return n;
    }
  }
  if (n->l != NULL) {
    node_t *result = bspGetNodeForLineSub(n->l, a, b);
    if (result) return result;
  }
  if (n->r != NULL) return bspGetNodeForLineSub(n->r, a, b);
  return 0;
}

static node_t *
bspGetNodeForLine(unsigned a, unsigned b)
{
  return root ? bspGetNodeForLineSub(root, a, b) : 0;
}

static line_t *linepool = NULL;

static void bspFreeTree() {
  if (linepool != NULL) {
    mmFree(linepool);
    linepool = NULL;
  }
  if (root != NULL) {
    mmFree(root);
    root = NULL;
  }
  cn = NULL;
  if (vc.p != NULL) {
    mmFree(vc.p);
    vc.p = NULL;
  }
  vc.n = 0;
}

static int
bspReadVertex(vertex_t *v, FILE *f)
{
  unsigned char buf[2 + 2], *p = buf;
  const size_t rd = fread(buf, 1, sizeof(buf), f);
  if (rd != sizeof(buf)) return -1;
  v->x = *(short *)p;
  v->y = *(short *)(p + 2);
  return 0;
}

static int
bspReadLine(line_t *l, FILE *f)
{
  unsigned char buf[2 + 2 + 1 + 2 + 2 + 2 + 4], *p = buf;
  const size_t rd = fread(buf, 1, sizeof(buf), f);
  if (rd != sizeof(buf)) return -1;
  l->a = *(short *)p;
  l->b = *(short *)(p + 2);
  l->flags = lineflag_t(p[4]);
  l->u1 = (double)(*(unsigned short *)(p + 5)) * 1.0f / 64.0f;
  l->u2 = (double)(*(unsigned short *)(p + 7)) * 1.0f / 64.0f;
  l->v = *(unsigned short *)(p + 9);
  l->t = *(unsigned *)(p + 11);
  l->nn = 0;
  return 0;
}

struct bsp_load_ctx {
  FILE *f;
  unsigned rn, rl;
  int err;
  node_t *np;
  line_t *lp;
};

static node_t *
bspLoadNode(struct bsp_load_ctx * const blc)
{
  unsigned lineCount;
  if (!fread(&lineCount, sizeof(unsigned), 1, blc->f)) { ++blc->err; return NULL; }
  if (!lineCount) return NULL;
  if (!blc->rn) {
    cmsg(MLERR, "bspLoadNode: out of preallocated nodes");
    ++blc->err;
    return NULL;
  }
  --blc->rn;
  node_t *n = blc->np++;
  n->n = lineCount - 1;
  n->div.load(blc->f);
  unsigned someCount;
  if (!fread(&someCount, sizeof(unsigned), 1, blc->f)) { ++blc->err; return n; }
  if (someCount) {
    n->s = sc.p + someCount;
    texLoadTexture(GET_TEXTURE(n->s->t, 0), 0);
    texLoadTexture(GET_TEXTURE(n->s->t, 1), 0);
  } else {
    n->s = NULL;
  }
  n->p = NULL;
  if (n->n) {
    if (n->n > blc->rl) {
      cmsg(MLERR, "bspLoadNode: out of preallocated lines");
      ++blc->err;
      return n;
    }
    blc->rl -= n->n;
    n->p = blc->lp;
    blc->lp += n->n;
    for (unsigned i = 0; i < n->n; ++i) {
      if (bspReadLine(n->p + i, blc->f)) { ++blc->err; return n; }
      texLoadTexture(GET_TEXTURE(n->p[i].t, 0), 0);
      texLoadTexture(GET_TEXTURE(n->p[i].t, 1), 0);
      texLoadTexture(GET_TEXTURE(n->p[i].t, 2), 0);
    }
    for (unsigned i = 0; i < n->n; ++i) {
      const float x = vc.p[n->p[i].a].y - vc.p[n->p[i].b].y;
      const float y = vc.p[n->p[i].b].x - vc.p[n->p[i].a].x;
      float l = 1 / sqrtf(x * x + y * y);
      if (n->s->c < n->s->f) l = -l;
      n->p[i].nx = x * l;
      n->p[i].ny = y * l;
    }
  }
  n->l = bspLoadNode(blc);
  n->r = bspLoadNode(blc);
  n->ow = NULL;
  int j = 0;
  if (n->l != NULL) {
    n->bb.x1 = n->l->bb.x1;
    n->bb.y1 = n->l->bb.y1;
    n->bb.z1 = n->l->bb.z1;
    n->bb.x2 = n->l->bb.x2;
    n->bb.y2 = n->l->bb.y2;
    n->bb.z2 = n->l->bb.z2;
    j = 1;
  } else if (n->r != NULL) {
    n->bb.x1 = n->r->bb.x1;
    n->bb.y1 = n->r->bb.y1;
    n->bb.z1 = n->r->bb.z1;
    n->bb.x2 = n->r->bb.x2;
    n->bb.y2 = n->r->bb.y2;
    n->bb.z2 = n->r->bb.z2;
    j = 2;
  } else if (n->n) {
    n->bb.x1 = n->bb.x2 = vc.p[n->p[0].a].x;
    n->bb.y1 = n->bb.y2 = vc.p[n->p[0].a].y;
    n->bb.z1 = n->bb.z2 = n->s->f;
    j = 2;
  }
  switch (j) {
    case 1:
      if (n->r != NULL) {
        bbAdd(&n->bb, n->r->bb.x1, n->r->bb.y1, n->r->bb.z1);
        bbAdd(&n->bb, n->r->bb.x2, n->r->bb.y2, n->r->bb.z2);
      }
      /* intentionally no break here */
    case 2:
      for (unsigned i = 0; i < n->n; ++i) {
        bbAdd(&n->bb, vc.p[n->p[i].a].x, vc.p[n->p[i].a].y, n->s->f);
        bbAdd(&n->bb, vc.p[n->p[i].a].x, vc.p[n->p[i].a].y, n->s->c);
      }
      break;
    default:
      cmsg(MLERR, "bspLoadNode: unable to initialize bounding boxes");
      break;
  }
  return n;
}

static void
bspNeighSub(struct bsp_load_ctx * const blc, node_t *n)
{
  for (unsigned i = 0; i < n->n; ++i) {
    if ((n->p[i].flags & LF_TWOSIDED) && n->p[i].nn == NULL) {
      n->p[i].nn = bspGetNodeForLine(n->p[i].b, n->p[i].a);
    }
  }
  if (n->l != NULL) bspNeighSub(blc, n->l);
  if (n->r != NULL) bspNeighSub(blc, n->r);
}

static node_t *
bspGetContSub(struct bsp_load_ctx * const blc, node_t *n, node_t *m)
{
  if (n->s != NULL) {
    if (n->s->f > n->s->c) return 0;
    for (unsigned i = 0; i < m->n; ++i) {
      if (bbInside(&n->bb, vc.p[m->p[i].a].x, vc.p[m->p[i].a].y, m->s->c, 0.0) ||
          bbInside(&n->bb, vc.p[m->p[i].a].x, vc.p[m->p[i].a].y, m->s->f, 0.0)) {
        return n;
      }
    }
  }
  if (n->l != NULL) {
    node_t *result = bspGetContSub(blc, n->l, m);
    if (result) return result;
  }
  if (n->r != NULL) return bspGetContSub(blc, n->r, m);
  return 0;
}

static node_t *
bspGetContainerNode(struct bsp_load_ctx * const blc, node_t *m)
{
  return root ? bspGetContSub(blc, root, m) : 0;
}

static void
bspSearchOutsiders(struct bsp_load_ctx * const blc, node_t *n)
{
  if (n->s != NULL && n->s->f < n->s->c) return;
  if (n->l != NULL) bspSearchOutsiders(blc, n->l);
  if (n->r != NULL) bspSearchOutsiders(blc, n->r);
  if (n->n) {
    n->ow = bspGetContainerNode(blc, n);
//    cmsg(MLINFO, "outsider! %d %d", n->ow->s - sc.p, n->ow->n);
  }
}

static int
bspLoadTree(FILE *f) {
  struct bsp_load_ctx blc;
  blc.f = f;
  blc.err = 0;
  if (!fread(&vc.n, sizeof(int), 1, f)) return 0;
  vc.p = (vertex_t *)mmAlloc(vc.n * sizeof(vertex_t));
  if (vc.p == NULL) return 0;
  for (unsigned i = 0; i < vc.n; ++i) {
    if (bspReadVertex(&vc.p[i], f)) return 0;
  }
  cmsg(MLINFO, "%d verteces", vc.n);
  if (!fread(&blc.rn, sizeof(unsigned), 1, f)) return 0;
  if (!fread(&blc.rl, sizeof(unsigned), 1, f)) return 0;
  blc.np = root = (node_t *)mmAlloc(blc.rn * sizeof(node_t));
  if (root == NULL) return 0;
  blc.lp = linepool = (line_t *)mmAlloc(blc.rl * sizeof(line_t));
  if (linepool == NULL) return 0;
  bspLoadNode(&blc);
  cmsg(MLINFO, "%d nodes, %d lines", blc.np - root, blc.lp - linepool);
  cmsg(MLINFO, "%d textures", texGetNofTextures());
  bspNeighSub(&blc, root);
  bspSearchOutsiders(&blc, root);
  return !0;
}

void bspFreeMap() {
  cmsg(MLDBG, "bspFreeMap");
  if (bspLoaded) menuEnable();
  bspLoaded = 0;
  modelFlush();
  bspFreeTree();
  if (sc.p != NULL) {
    mmFree(sc.p);
    sc.p = NULL;
  }
  sc.n = 0;
  texFlush();
  gDisable();
}

static int
bspLoadSector(sector_t *s, FILE *fp)
{
  unsigned char buf[2 + 2 + 1 + 2 + 2 + 4], *p = buf;
  const size_t rd = fread(buf, 1, sizeof(buf), fp);
  if (rd != sizeof(buf)) return -1;
  s->f = *(short *)p;
  s->c = *(short *)(p + 2);
  s->l = p[4];
  s->u = *(short *)(p + 5);
  s->v = *(short *)(p + 7);
  s->t = *(unsigned *)(p + 9);
  return 0;
}

int bspLoadMap(const char *fname) {
  int ret = 0;
  FILE *f = fopen(fname, "rb");
  if (f == NULL) {
    cmsg(MLERR, "bspLoadMap: unable to open file %s for reading", fname);
    return ret;
  }
  cmsg(MLINFO, "Loading map %s", fname);
  bspFreeMap();
  if (!fread(&sc.n, sizeof(int), 1, f)) goto end;
  sc.p = (sector_t *)mmAlloc(sc.n * sizeof(sector_t));
  if (sc.p == NULL) goto end;
  for (unsigned i = 0; i < sc.n; ++i) {
    if (bspLoadSector(sc.p + i, f)) goto end;
  }
  cmsg(MLINFO, "%d sectors", sc.n);
  if (!bspLoadTree(f)) goto end;
  if (!objLoad(f)) goto end;
  ++ret;
 end:
  fclose(f);
  if (!ret) {
    bspFreeMap();
    cmsg(MLERR, "Loading failed");
  } else {
    cmsg(MLINFO, "Map successfuly loaded");
    bspLoaded = 1;
    gEnable();
  }
  texLoadReady();
  return ret;
}

int cmd_map(int argc, char **argv) {
  if (argc < 2) {
    cmsg(MLINFO, "usage: %s <mapname>", *argv);
    return !0;
  }
//  cheats = 0;
  char s[64];
  snprintf(s, 63, "data/maps/%s.bsp", argv[1]);
  s[63] = '\0';
  if (!bspLoadMap(s)) return !0;
//  if (!strcmp(*argv, "devmap")) ++cheats;
  return 0;
}

static int
cmd_leavemap(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  bspFreeMap();
  return 0;
}

void bspInit() {
  sc.p = NULL;
  sc.n = 0;
  vc.p = NULL;
  vc.n = 0;
  cmdAddCommand("map", cmd_map);
  cmdAddCommand("devmap", cmd_map);
  cmdAddCommand("leavemap", cmd_leavemap);
  cmdAddBool("maploaded", &bspLoaded, 0);
}

void bspDone() {
  bspFreeMap();
}
