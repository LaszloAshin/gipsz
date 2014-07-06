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
    return ((dy > 0) ? PI : -PI) - atanf(dy / -dx);
  else
    return (dy > 0) ? PI/2 : -PI/2;
}

/* function to handle point-bsptree collision */
void bspCollideTree(float x, float y, float z, float *dx, float *dy, float *dz, int hard) {
  unsigned i, j = 0;
  vertex_t p, f;
  float q, pz;
  line_t *l;
  float dx1, dy1, dx2, dy2;
  sector_t *ns;
  int front, in;

  void bspCollideNode(node_t *n) {
    if (n == NULL || !bbInside(&n->bb, x, y, z, 48.0)) return;
    bspCollideNode(n->l);
    bspCollideNode(n->r);
    if (n->s == NULL || !n->n) return;
    p.x = x + *dx;
    p.y = y + *dy;
    pz  = z + *dz;
    in = 1;
    for (i = n->n, l = n->p; i; --i, ++l) {
      dx1 = vc.p[l->b].x - vc.p[l->a].x;
      dy1 = vc.p[l->b].y - vc.p[l->a].y;
      dx2 = vc.p[l->b].x - x;
      dy2 = vc.p[l->b].y - y;
      front = dx1 * dy2 < dy1 * dx2;
      if (!front) in = 0;
      if (n->s->f < n->s->c) {
        if (pz < n->s->f - 16 - 8 || pz > n->s->c + 48 + 8) continue;
      } else {
        if (pz < n->s->c - 16 - 8 || pz > n->s->f + 48 + 8) continue;
      }
      if (pszt(vc.p[l->a], vc.p[l->b], p, &f) > 256.0) continue;
      in = 0;
      if (pz > n->s->f && pz < n->s->f + 48) {
        *dz = (n->s->f + 48 - z) * 0.1;
        pz  = z + *dz;
      }
      if (pz < n->s->c && pz > n->s->c - 16) {
        *dz = (n->s->c - 16 - z) * 0.1;
        pz  = z + *dz;
      }
      ns = (l->nn != NULL) ? l->nn->s : NULL;
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
      q = sqrtf(f.x * f.x + f.y * f.y);
      f.x /= q;  f.y /= q;
      if (hard) {
        if (j) {
          *dx += f.x;
          *dy += f.y;
        } else {
          *dx = f.x;
          *dy = f.y;
        }
        ++j;
        continue;
      }
      q = cosf(at(f.x, -f.y) - at(*dy, *dx)) * sqrtf(*dx * *dx + *dy * *dy);
      *dx = -f.y * q;
      *dy = f.x * q;
    }
    if (in) {
      if (pz > n->s->f && pz < n->s->f + 48) {
        *dz = (n->s->f + 48 - z) * 0.1;
        pz  = z + *dz;
      }
      if (pz < n->s->c && pz > n->s->c - 16) {
        *dz = (n->s->c - 16 - z) * 0.1;
        pz  = z + *dz;
      }
    }
  }

  if (root != NULL) bspCollideNode(root);
}

node_t *bspGetNodeForCoords(float x, float y, float z) {
  node_t *nod = NULL;
  int bo, i;
  line_t *l;
  vertex_t *a, *b;
  float dx1, dy1, dx2, dy2;

  void bspGetNodeForCoordsSub(node_t *n) {
    if (n->l == NULL && n->r == NULL && n->n && n->s->f < n->s->c && bbInside(&n->bb, x, y, z, 0.0)) {
      bo = 0;
      b = vc.p + n->p[0].a;
      for (i = n->n, l = n->p; i; --i, ++l) {
        a = b;
        b = vc.p + l->b;
        dx1 = b->x - a->x;
        dy1 = b->y - a->y;
        dx2 = b->x - x;
        dy2 = b->y - y;
        if (dx1 * dy2 > dy1 * dx2) {
          ++bo;
          break;
        }
      }
      if (!bo) {
        nod = n;
        return;
      }
    }
    if (n->l != NULL) bspGetNodeForCoordsSub(n->l);
    if (nod != NULL) return;
    if (n->r != NULL) bspGetNodeForCoordsSub(n->r);
  }

  if (root != NULL) bspGetNodeForCoordsSub(root);
  return nod;
}

static node_t *bspGetNodeForLine(unsigned a, unsigned b) {
  node_t *nod = NULL;
  int i;

  void bspGetNodeForLineSub(node_t *n) {
    for (i = 0; i < n->n; ++i)
      if ((n->p[i].flags & LF_TWOSIDED) && n->p[i].a == a && n->p[i].b == b) {
        nod = n;
        return;
      }
    if (n->l != NULL) bspGetNodeForLineSub(n->l);
    if (nod != NULL) return;
    if (n->r != NULL) bspGetNodeForLineSub(n->r);
  }

  if (root != NULL) bspGetNodeForLineSub(root);
  return nod;
}

typedef struct {
  short x : 16;
  short y : 16;
} __attribute__((packed)) fvertex_t;

typedef struct {
  short a : 16;
  short b : 16;
  unsigned char flags : 8;
  unsigned short u1 : 16;
  unsigned short u2 : 16;
  unsigned short v : 16;
  unsigned int t : 32;
} __attribute__((packed)) fline_t;

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

static int bspLoadTree(FILE *f) {
  fvertex_t fv;
  fline_t fl;
  int i, j;
  int rn, rl;
  int err = 0;
  node_t *np;
  line_t *lp;
  float x, y, l;

  node_t *bspLoadNode() {
    if (!fread(&i, sizeof(int), 1, f)) { ++err; return NULL; }
    if (!i) return NULL;
    if (!rn) {
      cmsg(MLERR, "bspLoadNode: out of preallocated nodes");
      ++err;
      return NULL;
    }
    --rn;
    node_t *n = np++;
    n->n = i - 1;
    if (!fread(&i, sizeof(int), 1, f)) { ++err; return n; }
    if (i) {
      n->s = sc.p + i;
      texLoadTexture(GET_TEXTURE(n->s->t, 0), 0);
      texLoadTexture(GET_TEXTURE(n->s->t, 1), 0);
    } else
      n->s = NULL;
    n->p = NULL;
    if (n->n) {
      if (n->n > rl) {
        cmsg(MLERR, "bspLoadNode: out of preallocated lines");
        ++err;
        return n;
      }
      rl -= n->n;
      n->p = lp;
      lp += n->n;
      for (i = 0; i < n->n; ++i) {
        if (!fread(&fl, sizeof(fline_t), 1, f)) { ++err; return n; }
        n->p[i].a = fl.a;
        n->p[i].b = fl.b;
        n->p[i].u1 = (double)fl.u1 * 0.015625;
        n->p[i].u2 = (double)fl.u2 * 0.015625;
        n->p[i].v = fl.v;
        n->p[i].flags = fl.flags;
        n->p[i].nn = NULL;
        n->p[i].t = fl.t;
        texLoadTexture(GET_TEXTURE(fl.t, 0), 0);
        texLoadTexture(GET_TEXTURE(fl.t, 1), 0);
        texLoadTexture(GET_TEXTURE(fl.t, 2), 0);
      }
      for (i = 0; i < n->n; ++i) {
        x = vc.p[n->p[i].a].y - vc.p[n->p[i].b].y;
        y = vc.p[n->p[i].b].x - vc.p[n->p[i].a].x;
        l = 1 / sqrtf(x * x + y * y);
        if (n->s->c < n->s->f) l = -l;
        n->p[i].nx = x * l;
        n->p[i].ny = y * l;
      }
    }
    n->l = bspLoadNode();
    n->r = bspLoadNode();
    n->ow = NULL;
    j = 0;
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
        for (i = 0; i < n->n; ++i) {
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

  void bspNeighSub(node_t *n) {
    for (i = 0; i < n->n; ++i)
      if ((n->p[i].flags & LF_TWOSIDED) && n->p[i].nn == NULL)
        n->p[i].nn = bspGetNodeForLine(n->p[i].b, n->p[i].a);
    if (n->l != NULL) bspNeighSub(n->l);
    if (n->r != NULL) bspNeighSub(n->r);
  }

  node_t *bspGetContainerNode(node_t *m) {
    node_t *r = NULL;
    int i;

    void bspGetContSub(node_t *n) {
      if (n->s != NULL) {
        if (n->s->f > n->s->c) return;
        for (i = 0; i < m->n; ++i) {
          if (bbInside(&n->bb, vc.p[m->p[i].a].x, vc.p[m->p[i].a].y, m->s->c, 0.0) ||
              bbInside(&n->bb, vc.p[m->p[i].a].x, vc.p[m->p[i].a].y, m->s->f, 0.0)) {
            r = n;
            return;
          }
        }
      }
      if (n->l != NULL) bspGetContSub(n->l);
      if (r != NULL) return;
      if (n->r != NULL) bspGetContSub(n->r);
    }

    if (root != NULL) bspGetContSub(root);
    return r;
  }

  void bspSearchOutsiders(node_t *n) {
    if (n->s != NULL && n->s->f < n->s->c) return;
    if (n->l != NULL) bspSearchOutsiders(n->l);
    if (n->r != NULL) bspSearchOutsiders(n->r);
    if (n->n) {
      n->ow = bspGetContainerNode(n);
//      cmsg(MLINFO, "outsider! %d %d", n->ow->s - sc.p, n->ow->n);
    }
  }

  if (!fread(&vc.n, sizeof(fvertex_t), 1, f)) return 0;
  vc.p = (vertex_t *)mmAlloc(vc.n * sizeof(vertex_t));
  if (vc.p == NULL) return 0;
  for (i = 0; i < vc.n; ++i) {
    if (!fread(&fv, sizeof(fvertex_t), 1, f)) return 0;
    vc.p[i].x = fv.x;
    vc.p[i].y = fv.y;
  }
  cmsg(MLINFO, "%d verteces", vc.n);
  if (!fread(&rn, sizeof(int), 1, f)) return 0;
  if (!fread(&rl, sizeof(int), 1, f)) return 0;
  np = root = (node_t *)mmAlloc(rn * sizeof(node_t));
  if (root == NULL) return 0;
  lp = linepool = (line_t *)mmAlloc(rl * sizeof(line_t));
  if (linepool == NULL) return 0;
  bspLoadNode();
  cmsg(MLINFO, "%d nodes, %d lines", np - root, lp - linepool);
  cmsg(MLINFO, "%d textures", texGetNofTextures());
  bspNeighSub(root);
  bspSearchOutsiders(root);
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
  if (fread(sc.p, sizeof(sector_t), sc.n, f) != sc.n) goto end;
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

int cmd_leavemap(int argc, char **argv) {
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
