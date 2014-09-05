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

bool
Line::crossable()
const
{
  if (!sectorBehind()) return false;
  if (sectorBehind()->height() < 64) return false;
  return true;
}

void
Leaf::collide(MassPoint3d& mp)
const
{
  const Vec3d newPos(mp.pos() + mp.velo());
  for (const_iterator i(begin()); i != end(); ++i) {
    const Vec2d pm(i->nearestPoint(newPos.xy()));
    const Vec2d n2d(norm(newPos.xy() - pm));
    const Vec3d n3d(n2d.x(), n2d.y(), 0.0f);
    const double d = len(newPos.xy() - pm) - 16.0f;
    if (d < 0.0f) {
      if (!i->crossable()) {
        mp.velo(mp.velo() - d * n3d);
      }
    }
  }
  const double df = newPos.z() - s()->f() - 48.0f;
  if (df < 0.0f) {
    mp.velo(mp.velo() - Vec3d(0.0f, 0.0f, df * 0.1f));
  }
}

void bspCollideTree(MassPoint3d& mp) {
  if (!root.get()) return;
  if (const Leaf* l = bspGetLeafForCoords(mp.pos())) {
    l->collide(mp);
  }
}

const Leaf*
Branch::findLeaf(const Vec3d& p)
const
{
  if (div().determine(p) < thickness) {
    if (const Leaf* result = back()->findLeaf(p)) return result;
  }
  return front()->findLeaf(p);
}

const Leaf*
Leaf::findLeaf(const Vec3d& p)
const
{
  if (p.z() < s()->f() || s()->c() < p.z()) return 0;
  for (const_iterator i(begin()); i != end(); ++i) {
    if (isBehind(p.xy(), *i)) return 0;
  }
  return this;
}

const Leaf* bspGetLeafForCoords(const Vec3d& p) {
  return root.get() ? root->findLeaf(p) : 0;
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
bspReadLine(std::istream& is, const Sectors& sectors)
{
  std::string name;
  is >> name;
  if (name != "wall") throw std::runtime_error("wall expected");
  Vec2d a, b;
  unsigned flags;
  int backSectorId;
  is >> a >> b >> flags >> backSectorId;
  std::tr1::shared_ptr<Sector> sectorBehind;
  if (backSectorId) sectorBehind = sectors.at(backSectorId);
  Surfaced s;
  is >> s;
  return Line(a, b, flags, sectorBehind, s);
}

std::auto_ptr<Node>
Leaf::create(std::istream& is, const Sectors& sectors)
{
  std::string name;
  is >> name;
  if (name != "subsector") throw std::runtime_error("subsector expected");
  unsigned sectorId;
  is >> sectorId;
  printf("sectorId: %u\n", sectorId);
  assert(sectorId); // XXX
  std::auto_ptr<Leaf> result(new Leaf(sectors.at(sectorId)));
  texLoadTexture(GET_TEXTURE(result->s()->t(), 0), 0);
  texLoadTexture(GET_TEXTURE(result->s()->t(), 1), 0);
  size_t lineCount;
  is >> lineCount;
  assert(lineCount);
  if (lineCount) { // leaf
    result->ls().reserve(lineCount);
    for (size_t i = 0; i < lineCount; ++i) {
      Line l(bspReadLine(is, sectors));
      texLoadTexture(GET_TEXTURE(l.s().textureId(), 0), 0);
      texLoadTexture(GET_TEXTURE(l.s().textureId(), 1), 0);
      texLoadTexture(GET_TEXTURE(l.s().textureId(), 2), 0);
      l.n(norm(perpendicular(l.b() - l.a())));
      result->bb().add(Vec3d(l.a().x(), l.a().y(), result->s()->f()));
      result->bb().add(Vec3d(l.a().x(), l.a().y(), result->s()->c()));
      result->ls().push_back(l);
    }
  }
  return std::auto_ptr<Node>(result.release());
}

static std::auto_ptr<Node>
bspLoadNode(std::istream& is, const Sectors& sectors)
{
  std::string name;
  is >> name;
  if (name != "node") throw std::runtime_error("node expected");
  std::string branchOrLeaf;
  is >> branchOrLeaf;
  if (branchOrLeaf != "branch" && branchOrLeaf != "leaf") throw std::runtime_error("node is not branch nor leaf");
  return (branchOrLeaf == "leaf") ? Leaf::create(is, sectors) : Branch::create(is, sectors);
}

Branch::Branch(const Plane2d& div, std::auto_ptr<Node>& front, std::auto_ptr<Node>& back)
: div_(div)
, front_(front)
, back_(back)
{
  bb().add(front_->bb());
  bb().add(back_->bb());
}

std::auto_ptr<Node>
Branch::create(std::istream& is, const Sectors& sectors)
{
  const Plane2d plane(Plane2d::read(is));
  std::auto_ptr<Node> front(bspLoadNode(is, sectors));
  std::auto_ptr<Node> back(bspLoadNode(is, sectors));
  return std::auto_ptr<Node>(new Branch(plane, front, back));
}

void bspFreeMap() {
  cmsg(MLDBG, "bspFreeMap");
  if (bspLoaded) menuEnable();
  bspLoaded = 0;
  modelFlush();
  bspFreeTree();
  texFlush();
  gDisable();
}

static std::tr1::shared_ptr<Sector>
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
  return std::tr1::shared_ptr<Sector>(new Sector(f, c, l, u, v, t));
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
  Sectors sectors;
  sectors.reserve(sectorCount);
  for (size_t i = 0; i < sectorCount; ++i) {
    sectors.push_back(bspLoadSector(f, i));
  }
  cmsg(MLINFO, "%zu sectors", sectors.size());
  root = bspLoadNode(f, sectors);
  // TODO: count lines and nodes and print!
  cmsg(MLINFO, "%d textures", texGetNofTextures());
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
  cmdAddCommand("map", cmd_map);
  cmdAddCommand("devmap", cmd_map);
  cmdAddCommand("leavemap", cmd_leavemap);
  cmdAddBool("maploaded", &bspLoaded, 0);
}

void bspDone() {
  bspFreeMap();
}
