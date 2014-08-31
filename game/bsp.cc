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

static const double thickness = 0.1f;

Vertexes vc;
sc_t sc;

node_t *root = NULL, *cn = NULL;

static int bspLoaded = 0;
int bspIsLoaded() { return bspLoaded; }

double clamp(double value, double low, double high) { return (value < low) ? low : ((value > high) ? high : value); }

static Vec2d
nearestWallPoint(const line_t& l, const Vec2d& pm)
{
  const Vec2d p1(vc[l.a]);
  const Vec2d p2(vc[l.b]);
  const Vec2d d(p2 - p1);
  const double t = clamp(dot(pm - p1, d) / dot(d, d), 0.0f, 1.0f);
  return p1 + t * d;
}

static bool
pointBehindLine(const line_t& l, const Vec2d& p0)
{
  const Vec2d p1(vc[l.a]);
  const Vec2d p2(vc[l.b]);
  return wedge(p2 - p1, p0 - p1) < 0.0f;
}

static bool
crossable(const line_t& l)
{
  if (!l.backSectorId) return false;
  const sector_t& neighSector(sc.p[l.backSectorId]);
  if (neighSector.c - neighSector.f < 64) return false;
  return true;
}

static void
bspCollideNode(const node_t* n, MassPoint3d& mp)
{
  const Vec3d newPos(mp.pos() + mp.velo());
  for (line_t* l = n->p; l < n->p + n->n; ++l) {
    const Vec2d pm(nearestWallPoint(*l, newPos.xy()));
    const Vec2d n2d(norm(newPos.xy() - pm));
    const Vec3d n3d(n2d.x(), n2d.y(), 0.0f);
    const double d = len(newPos.xy() - pm) - 16.0f;
    if (d < 0.0f) {
      if (!crossable(*l)) {
        mp.velo(mp.velo() - d * n3d);
      }
    }
  }
  const double df = newPos.z() - n->s->f - 48.0f;
  if (df < 0.0f) {
    mp.velo(mp.velo() - df * Vec3d(0.0f, 0.0f, 1.0f) / 10);
  }
}

void bspCollideTree(MassPoint3d& mp) {
  if (!root) return;
  if (const node_t* n = bspGetNodeForCoords(mp.pos())) {
    bspCollideNode(n, mp);
  }
}

static node_t *
bspGetNodeForCoordsSub(node_t *n, const Vec3d& p)
{
  if (n->n) { // leaf
    if (p.z() < n->s->f || n->s->c < p.z()) return 0;
    for (line_t* l = n->p; l < n->p + n->n; ++l) {
      if (pointBehindLine(*l, p.xy())) return 0;
    }
    return n;
  } else {
    if (n->div.determine(p) < thickness) {
      if (node_t* result = bspGetNodeForCoordsSub(n->r, p)) return result;
    }
    return bspGetNodeForCoordsSub(n->l, p);
  }
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
  vc.clear();
}

static Vertex
bspReadVertex(std::istream& is, size_t expectedIndex)
{
  std::string name;
  is >> name;
  if (name != "vertex") throw std::runtime_error("vertex expected");
  size_t index;
  is >> index;
  if (index != expectedIndex) throw std::runtime_error("unexpected index");
  float x, y;
  is >> x >> y;
  return Vertex(x, y);
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
        const float x = vc[n->p[i].a].y() - vc[n->p[i].b].y();
        const float y = vc[n->p[i].b].x() - vc[n->p[i].a].x();
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
    n->bb.add(Vec3d(vc[n->p[0].a].x(), vc[n->p[0].a].y(), n->s->f));
    j = 2;
  }
  switch (j) {
    case 1:
      if (n->r) n->bb.add(n->r->bb);
      /* intentionally no break here */
    case 2:
      for (unsigned i = 0; i < n->n; ++i) {
        n->bb.add(Vec3d(vc[n->p[i].a].x(), vc[n->p[i].a].y(), n->s->f));
        n->bb.add(Vec3d(vc[n->p[i].a].x(), vc[n->p[i].a].y(), n->s->c));
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
      if (n->bb.inside(Vec3d(vc[m->p[i].a].x(), vc[m->p[i].a].y(), m->s->c)) ||
          n->bb.inside(Vec3d(vc[m->p[i].a].x(), vc[m->p[i].a].y(), m->s->f))) {
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
  size_t vertexCount;
  is >> vertexCount;
  Vertexes().swap(vc);
  vc.reserve(vertexCount);
  for (unsigned i = 0; i < vertexCount; ++i) {
    vc.push_back(bspReadVertex(is, i));
  }
  cmsg(MLINFO, "%zu verteces", vc.size());
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
  vc.clear();
  cmdAddCommand("map", cmd_map);
  cmdAddCommand("devmap", cmd_map);
  cmdAddCommand("leavemap", cmd_leavemap);
  cmdAddBool("maploaded", &bspLoaded, 0);
}

void bspDone() {
  bspFreeMap();
}
