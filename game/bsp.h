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
