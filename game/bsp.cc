/* vim: set ts=2 sw=8 tw=0 et :*/
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

#include <cassert>
#include <istream>
#include <fstream>
#include <stdexcept>

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
  Vec3d p;
  float *dx, *dy, *dz;
  int hard;
  int j;
};

static void
bspCollideNode(struct collide_param_rec *pr, node_t *n)
{
  if (n == NULL || !n->bb.inside(pr->p, Vec3d(48.0f, 48.0f, 48.0f))) return;
  bspCollideNode(pr, n->l);
  bspCollideNode(pr, n->r);
  if (n->s == NULL || !n->n) return;
  vertex_t p;
  p.x = pr->p.x() + *pr->dx;
  p.y = pr->p.y() + *pr->dy;
  float pz = pr->p.z() + *pr->dz;
  int in = 1;
  for (line_t *l = n->p; l < n->p + n->n; ++l) {
    float dx1 = vc.p[l->b].x - vc.p[l->a].x;
    const float dy1 = vc.p[l->b].y - vc.p[l->a].y;
    float dx2 = vc.p[l->b].x - pr->p.x();
    const float dy2 = vc.p[l->b].y - pr->p.y();
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
      *pr->dz = (n->s->f + 48 - pr->p.z()) * 0.1;
      pz = pr->p.z() + *pr->dz;
    }
    if (pz < n->s->c && pz > n->s->c - 16) {
      *pr->dz = (n->s->c - 16 - pr->p.z()) * 0.1;
      pz = pr->p.z() + *pr->dz;
    }
    const sector_t* const ns = l->backSectorId ? (sc.p + l->backSectorId) : 0;
    if (ns) {
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
      *pr->dz = (n->s->f + 48 - pr->p.z()) * 0.1;
      pz = pr->p.z() + *pr->dz;
    }
    if (pz < n->s->c && pz > n->s->c - 16) {
      *pr->dz = (n->s->c - 16 - pr->p.z()) * 0.1;
      pz = pr->p.z() + *pr->dz;
    }
  }
}

/* function to handle point-bsptree collision */
void bspCollideTree(const Vec3d& p, float *dx, float *dy, float *dz, int hard) {
  struct collide_param_rec pr;
  pr.p = p;
  pr.dx = dx;
  pr.dy = dy;
  pr.dz = dz;
  pr.hard = hard;
  pr.j = 0;
  if (root != NULL) bspCollideNode(&pr, root);
}

static node_t *
bspGetNodeForCoordsSub(node_t *n, const Vec3d& p)
{
  if (n->l == NULL && n->r == NULL && n->n && n->s->f < n->s->c && n->bb.inside(p)) {
    int bo = 0;
    vertex_t *b = vc.p + n->p[0].a;
    for (line_t *l = n->p; l < n->p + n->n; ++l) {
      vertex_t *a = b;
      b = vc.p + l->b;
      const float dx1 = b->x - a->x;
      const float dy1 = b->y - a->y;
      const float dx2 = b->x - p.x();
      const float dy2 = b->y - p.y();
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
    node_t *result = bspGetNodeForCoordsSub(n->l, p);
    if (result) return result;
  }
  if (n->r != NULL) {
    return bspGetNodeForCoordsSub(n->r, p);
  }
  return 0;
}

node_t *bspGetNodeForCoords(const Vec3d& p) {
  return root ? bspGetNodeForCoordsSub(root, p) : 0;
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

static void
bspReadVertex(vertex_t *v, std::istream& is, size_t expectedIndex)
{
  std::string name;
  is >> name;
  if (name != "vertex") throw std::runtime_error("vertex expected");
  size_t index;
  is >> index;
  if (index != expectedIndex) throw std::runtime_error("unexpected index");
  is >> v->x >> v->y;
}

static void
bspReadLine(line_t *l, std::istream& is)
{
  std::string name;
  is >> name;
  if (name != "wall") throw std::runtime_error("wall expected");
  is >> l->a;
  is >> l->b;
  int i;
  is >> i;
  l->flags = lineflag_t(i);
  is >> l->backSectorId;
  is >> name;
  if (name != "surface") throw std::runtime_error("surface expected");
  is >> l->u1;
  is >> l->u2;
  is >> l->v;
  is >> l->t;
  l->u1 /= 64.0f;
  l->u2 /= 64.0f;
}

struct bsp_load_ctx {
  std::istream& is;
  unsigned rn, rl;
  node_t *np;
  line_t *lp;

  bsp_load_ctx(std::istream& is0) : is(is0), rn(0), rl(0), np(0), lp(0) {}
};

static node_t *
bspLoadNode(struct bsp_load_ctx * const blc, size_t level)
{
  std::string name;
  blc->is >> name;
  if (name != "node") throw std::runtime_error("node expected");
  std::string branchOrLeaf;
  blc->is >> branchOrLeaf;
  if (branchOrLeaf != "branch" && branchOrLeaf != "leaf") throw std::runtime_error("node is not branch nor leaf");
  const int isLeaf = (branchOrLeaf == "leaf");
  for (size_t i = 0; i < level * 2; ++i) putchar(' ');
  printf("## node is %s\n", isLeaf ? "leaf" : "not leaf");
  if (!blc->rn) throw std::runtime_error("out of preallocated nodes");
  --blc->rn;
  node_t* n = blc->np++;
  n->p = 0;
  n->n = 0;
  n->bb = BBox3d();
  n->s = 0;
  n->l = n->r = 0;
  n->ow = 0;
  if (isLeaf) {
    blc->is >> name;
    if (name != "subsector") throw std::runtime_error("subsector expected");
    unsigned sectorId;
    blc->is >> sectorId;
    printf("sectorId: %u\n", sectorId);
    if (sectorId) {
      n->s = sc.p + sectorId;
      texLoadTexture(GET_TEXTURE(n->s->t, 0), 0);
      texLoadTexture(GET_TEXTURE(n->s->t, 1), 0);
    }
    unsigned lineCount;
    blc->is >> lineCount;
    assert(lineCount);
    n->n = lineCount;
    if (n->n) {
      if (n->n > blc->rl) throw std::runtime_error("out of preallocated lines");
      blc->rl -= n->n;
      n->p = blc->lp;
      blc->lp += n->n;
      for (unsigned i = 0; i < n->n; ++i) {
        bspReadLine(n->p + i, blc->is);
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
  } else {
    n->div = Plane2d::read(blc->is);
    n->l = bspLoadNode(blc, level + 1);
    n->r = bspLoadNode(blc, level + 1);
  }



  int j = 0;
  if (n->l) {
    n->bb = n->l->bb;
    j = 1;
  } else if (n->r) {
    n->bb = n->r->bb;
    j = 2;
  } else if (n->n) {
    n->bb.add(Vec3d(vc.p[n->p[0].a].x, vc.p[n->p[0].a].y, n->s->f));
    j = 2;
  }
  switch (j) {
    case 1:
      if (n->r) n->bb.add(n->r->bb);
      /* intentionally no break here */
    case 2:
      for (unsigned i = 0; i < n->n; ++i) {
        n->bb.add(Vec3d(vc.p[n->p[i].a].x, vc.p[n->p[i].a].y, n->s->f));
        n->bb.add(Vec3d(vc.p[n->p[i].a].x, vc.p[n->p[i].a].y, n->s->c));
      }
      break;
    default:
      cmsg(MLERR, "bspLoadNode: unable to initialize bounding boxes");
      break;
  }
  return n;
}

static node_t *
bspGetContSub(struct bsp_load_ctx * const blc, node_t *n, node_t *m)
{
  if (n->s != NULL) {
    if (n->s->f > n->s->c) return 0;
    for (unsigned i = 0; i < m->n; ++i) {
      if (n->bb.inside(Vec3d(vc.p[m->p[i].a].x, vc.p[m->p[i].a].y, m->s->c)) ||
          n->bb.inside(Vec3d(vc.p[m->p[i].a].x, vc.p[m->p[i].a].y, m->s->f))) {
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

static void
bspLoadTree(std::istream& is) {
  struct bsp_load_ctx blc(is);
  std::string name;
  is >> name;
  if (name != "vertex-count") throw std::runtime_error("vertex-count expected");
  is >> vc.n;
  vc.p = (vertex_t *)mmAlloc(vc.n * sizeof(vertex_t));
  if (!vc.p) throw std::runtime_error("memory");
  for (unsigned i = 0; i < vc.n; ++i) {
    bspReadVertex(&vc.p[i], is, i);
  }
  cmsg(MLINFO, "%d verteces", vc.n);
  is >> name;
  if (name != "node-count") throw std::runtime_error("node-count expected");
  is >> blc.rn;
  is >> name;
  if (name != "line-count") throw std::runtime_error("line-count expected");
  is >> blc.rl;
  cmsg(MLINFO, "%d nodes, %d lines", blc.rn, blc.rl);
  blc.np = root = (node_t *)mmAlloc(blc.rn * sizeof(node_t));
  if (!root) throw std::runtime_error("memory");
  blc.lp = linepool = (line_t *)mmAlloc(blc.rl * sizeof(line_t));
  if (!linepool) throw std::runtime_error("memory");
  bspLoadNode(&blc, 0);
  cmsg(MLINFO, "really %d nodes, %d lines", blc.np - root, blc.lp - linepool);
  cmsg(MLINFO, "%d textures", texGetNofTextures());
  bspSearchOutsiders(&blc, root);
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
bspLoadSector(sector_t *s, std::istream& is, size_t expectedIndex)
{
  std::string name;
  is >> name;
  if (name != "sector") throw std::runtime_error("sector expected");
  size_t index;
  is >> index;
  if (index != expectedIndex) throw std::runtime_error("unexpected index");
  is >> s->f;
  is >> s->c;
  is >> s->l;
  is >> s->u;
  is >> s->v;
  is >> s->t;
  return 0;
}

int bspLoadMap(const char *fname) {
  std::ifstream f(fname);
  f.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
  std::string magic, version;
  f >> magic >> version;
  if (magic != "bsp") throw std::runtime_error("invalid magic");
  if (version != "v0.1") throw std::runtime_error("invalid version");
  cmsg(MLINFO, "Loading map %s", fname);
  bspFreeMap();
  std::string name;
  f >> name;
  if (name != "sector-count") throw std::runtime_error("sector-count expected");
  f >> sc.n;
  sc.p = (sector_t *)mmAlloc(sc.n * sizeof(sector_t));
  if (!sc.p) throw std::runtime_error("memory");
  for (unsigned i = 0; i < sc.n; ++i) {
    bspLoadSector(sc.p + i, f, i);
  }
  cmsg(MLINFO, "%d sectors", sc.n);
  bspLoadTree(f);
  objLoad(f);
  cmsg(MLINFO, "Map successfuly loaded");
  bspLoaded = 1;
  gEnable();
  texLoadReady();
  return !0;
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
