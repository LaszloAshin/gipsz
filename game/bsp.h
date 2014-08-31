#ifndef _BSP_H
#define _BSP_H 1

#include <game/bbox.hh>
#include <lib/vec.hh>
#include <lib/plane.hh>
#include <lib/masspoint.hh>

#include <cstdio>
#include <istream>
#include <vector>

typedef Vec2d Vertex;
typedef std::vector<Vertex> Vertexes;

class Sector {
public:
  Sector(short f, short c, unsigned l, short u, short v, unsigned t) : f_(f), c_(c), l_(l), u_(u), v_(v), t_(t) {}

  short f() const { return f_; }
  short c() const { return c_; }
  unsigned l() const { return l_; }
  short u() const { return u_; }
  short v() const { return v_; }
  unsigned t() const { return t_; }

  short height() const { return c() - f(); }

private:
  short f_, c_;
  unsigned l_;
  short u_, v_;
  unsigned t_;
};

typedef std::vector<Sector> Sectors;

struct LineFlag {
	enum Type {
		NOTHING = 0x00,
		TWOSIDED = 0x01,
		TOPSTART = 0x02,
		BOTTOMSTART = 0x04,
		BLOCK = 0x08
	};
};

typedef struct {
  unsigned a, b;
  float u1, u2, v;
  LineFlag::Type flags;
  Vec2d n;
  unsigned t;
  int backSectorId;
} line_t;

typedef struct node_s {
  line_t *p;
  unsigned n;
  BBox3d bb;
  Sector* s;
  struct node_s *l, *r, *ow;
  Plane2d div;
} node_t;

extern node_t *root, *cn;
extern Vertexes vc;
extern Sectors sc;

void bspCollideTree(MassPoint3d& mp);
node_t *bspGetNodeForCoords(const Vec3d& p);
void bspFreeMap();
int bspLoadMap(const char *fname);
void bspInit();
void bspDone();
int bspIsLoaded();

#endif /* _BSP_H */
