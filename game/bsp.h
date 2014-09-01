#ifndef _BSP_H
#define _BSP_H 1

#include <game/bbox.hh>
#include <lib/vec.hh>
#include <lib/plane.hh>
#include <lib/masspoint.hh>
#include <lib/surface.hh>

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

class Line {
public:
  Line(
    unsigned a, unsigned b,
    unsigned flags,
    int backSectorId,
    const Surfaced& s
  )
  : a_(a), b_(b)
  , flags_(flags)
  , backSectorId_(backSectorId)
  , s_(s)
  {}

  unsigned a() const { return a_; }
  unsigned b() const { return b_; }
  Vec2d n() const { return n_; }
  int backSectorId() const { return backSectorId_; }
  Surfaced s() const { return s_; }

  void n(const Vec2d& value) { n_ = value; }

private:
  unsigned a_, b_;
  unsigned flags_;
  Vec2d n_;
  int backSectorId_;
  Surfaced s_;
};

typedef std::vector<Line> Lines;

typedef struct node_s {
  Lines ls;
  BBox3d bb;
  Sector* s;
  struct node_s *front, *back;
  Plane2d div;

  typedef Lines::const_iterator const_iterator;
  typedef Lines::const_reverse_iterator const_reverse_iterator;

  const_iterator begin() const { return ls.begin(); }
  const_iterator end() const { return ls.end(); }
  const_reverse_iterator rbegin() const { return ls.rbegin(); }
  const_reverse_iterator rend() const { return ls.rend(); }
} node_t;

extern node_t *root;
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
