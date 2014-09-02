/* vim: set ts=2 sw=8 tw=0 et :*/
#include "bsp.h"
#include "console.h"
#include "tex.h"
#include "render.h"
#include "cmd.h"
#include "game.h"
#include "menu.h"
#include "model.h"
#include "obj.h"

#include <cassert>
#include <fstream>
#include <stdexcept>

static const double thickness = 0.1f;

Sectors sc;

std::auto_ptr<Node> root;

static int bspLoaded = 0;
int bspIsLoaded() { return bspLoaded; }

double clamp(double value, double low, double high) { return (value < low) ? low : ((value > high) ? high : value); }

Vec2d
Line::nearestPoint(const Vec2d& pm)
const
{
  const Vec2d d(b() - a());
  const double t = clamp(dot(pm - a(), d) / dot(d, d), 0.0f, 1.0f);
  return a() + t * d;
}

bool
isBehind(const Vec2d& p, const Line& l)
{
  return wedge(l.b() - l.a(), p - l.a()) < 0.0f;
}

static bool
crossable(const Line& l)
{
  if (!l.backSectorId()) return false;
  const Sector neighSector(sc[l.backSectorId()]);
  if (neighSector.height() < 64) return false;
  return true;
}

void
Node::collide(MassPoint3d& mp)
const
{
  const Vec3d newPos(mp.pos() + mp.velo());
  for (Node::const_iterator i(begin()); i != end(); ++i) {
    const Vec2d pm(i->nearestPoint(newPos.xy()));
    const Vec2d n2d(norm(newPos.xy() - pm));
    const Vec3d n3d(n2d.x(), n2d.y(), 0.0f);
    const double d = len(newPos.xy() - pm) - 16.0f;
    if (d < 0.0f) {
      if (!crossable(*i)) {
        mp.velo(mp.velo() - d * n3d);
      }
    }
  }
  const double df = newPos.z() - s()->f() - 48.0f;
  if (df < 0.0f) {
    mp.velo(mp.velo() - df * Vec3d(0.0f, 0.0f, 1.0f) / 10);
  }
}

void bspCollideTree(MassPoint3d& mp) {
  if (!root.get()) return;
  if (const Node* n = bspGetNodeForCoords(mp.pos())) {
    n->collide(mp);
  }
}

const Node*
Node::findNode(const Vec3d& p)
const
{
  if (!empty()) { // leaf
    if (p.z() < s()->f() || s()->c() < p.z()) return 0;
    for (Node::const_iterator i(begin()); i != end(); ++i) {
      if (isBehind(p.xy(), *i)) return 0;
    }
    return this;
  } else {
    if (div().determine(p) < thickness) {
      if (const Node* result = back()->findNode(p)) return result;
    }
    return front()->findNode(p);
  }
}

const Node* bspGetNodeForCoords(const Vec3d& p) {
  return root.get() ? root->findNode(p) : 0;
}

static void bspFreeTree() {
  root.reset();
}

namespace {

std::istream&
operator>>(std::istream& is, Vec2d& v)
{
  double x, y;
  is >> x >> y;
  v = Vec2d(x, y);
  return is;
}

} // anonymous namespace

static Line
bspReadLine(std::istream& is)
{
  std::string name;
  is >> name;
  if (name != "wall") throw std::runtime_error("wall expected");
  Vec2d a, b;
  unsigned flags;
  int backSectorId;
  is >> a >> b >> flags >> backSectorId;
  Surfaced s;
  is >> s;
  return Line(a, b, flags, backSectorId, s);
}

static std::auto_ptr<Node>
bspLoadNode(std::istream& is)
{
  std::string name;
  is >> name;
  if (name != "node") throw std::runtime_error("node expected");
  std::string branchOrLeaf;
  is >> branchOrLeaf;
  if (branchOrLeaf != "branch" && branchOrLeaf != "leaf") throw std::runtime_error("node is not branch nor leaf");
  const int isLeaf = (branchOrLeaf == "leaf");
  std::auto_ptr<Node> n(new Node());
  if (isLeaf) {
    is >> name;
    if (name != "subsector") throw std::runtime_error("subsector expected");
    unsigned sectorId;
    is >> sectorId;
    printf("sectorId: %u\n", sectorId);
    if (sectorId) {
      n->s(&sc.at(sectorId));
      texLoadTexture(GET_TEXTURE(n->s()->t(), 0), 0);
      texLoadTexture(GET_TEXTURE(n->s()->t(), 1), 0);
    }
    size_t lineCount;
    is >> lineCount;
    assert(lineCount);
    if (lineCount) { // leaf
      n->ls().reserve(lineCount);
      for (size_t i = 0; i < lineCount; ++i) {
        Line l(bspReadLine(is));
        texLoadTexture(GET_TEXTURE(l.s().textureId(), 0), 0);
        texLoadTexture(GET_TEXTURE(l.s().textureId(), 1), 0);
        texLoadTexture(GET_TEXTURE(l.s().textureId(), 2), 0);
        const float x = l.a().y() - l.b().y();
        const float y = l.b().x() - l.a().x();
        float len = 1 / sqrtf(x * x + y * y);
        l.n(Vec2d(x * len, y * len));
        n->bb().add(Vec3d(l.a().x(), l.a().y(), n->s()->f()));
        n->bb().add(Vec3d(l.a().x(), l.a().y(), n->s()->c()));
        n->ls().push_back(l);
      }
    }
  } else {
    n->div(Plane2d::read(is));
    n->front(bspLoadNode(is));
    n->back(bspLoadNode(is));
    n->bb().add(n->front()->bb());
    n->bb().add(n->back()->bb());
  }
  return n;
}

static void
bspLoadTree(std::istream& is) {
  root = bspLoadNode(is);
  // TODO: count lines and nodes and print!
  cmsg(MLINFO, "%d textures", texGetNofTextures());
}

void bspFreeMap() {
  cmsg(MLDBG, "bspFreeMap");
  if (bspLoaded) menuEnable();
  bspLoaded = 0;
  modelFlush();
  bspFreeTree();
  sc.clear();
  texFlush();
  gDisable();
}

static Sector
bspLoadSector(std::istream& is, size_t expectedIndex)
{
  std::string name;
  is >> name;
  if (name != "sector") throw std::runtime_error("sector expected");
  size_t index;
  is >> index;
  if (index != expectedIndex) throw std::runtime_error("unexpected index");
  short f, c, u, v;
  unsigned l, t;
  is >> f >> c >> l >> u >> v >> t;
  return Sector(f, c, l, u, v, t);
}

int bspLoadMap(const char *fname) {
  std::ifstream f(fname);
  f.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
  std::string magic, version;
  f >> magic >> version;
  if (magic != "bsp") throw std::runtime_error("invalid magic");
  if (version != "v0.2") throw std::runtime_error("invalid version");
  cmsg(MLINFO, "Loading map %s", fname);
  bspFreeMap();
  std::string name;
  f >> name;
  if (name != "sector-count") throw std::runtime_error("sector-count expected");
  size_t sectorCount;
  f >> sectorCount;
  Sectors().swap(sc);
  sc.reserve(sectorCount);
  for (size_t i = 0; i < sectorCount; ++i) {
    sc.push_back(bspLoadSector(f, i));
  }
  cmsg(MLINFO, "%zu sectors", sc.size());
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
  sc.clear();
  cmdAddCommand("map", cmd_map);
  cmdAddCommand("devmap", cmd_map);
  cmdAddCommand("leavemap", cmd_leavemap);
  cmdAddBool("maploaded", &bspLoaded, 0);
}

void bspDone() {
  bspFreeMap();
}
