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
#include <memory>

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

class Node {
public:
  Node() : s_(0), front_(0), back_(0) {}

  typedef Lines::const_iterator const_iterator;
  typedef Lines::const_reverse_iterator const_reverse_iterator;

  const_iterator begin() const { return ls_.begin(); }
  const_iterator end() const { return ls_.end(); }
  const_reverse_iterator rbegin() const { return ls_.rbegin(); }
  const_reverse_iterator rend() const { return ls_.rend(); }

  bool empty() const { return ls_.empty(); }
  const Sector* s() const { return s_; }
  Plane2d div() const { return div_; }
  const std::auto_ptr<Node>& front() const { return front_; }
  const std::auto_ptr<Node>& back() const { return back_; }
  BBox3d bb() const { return bb_; }
  BBox3d& bb() { return bb_; }

  void s(Sector* value) { s_ = value; }
  Lines& ls() { return ls_; }
  const Lines& ls() const { return ls_; }
  void div(const Plane2d& p) { div_ = p; }
  void front(std::auto_ptr<Node> value) { front_ = value; }
  void back(std::auto_ptr<Node> value) { back_ = value; }
  void bb(const BBox3d& value) { bb_ = value; }

private:
  Node(const Node&);
  Node& operator=(const Node&);

  Lines ls_;
  BBox3d bb_;
  Sector* s_;
  std::auto_ptr<Node> front_, back_;
  Plane2d div_;
};

extern std::auto_ptr<Node> root;
extern Vertexes vc;
extern Sectors sc;

void bspCollideTree(MassPoint3d& mp);
Node* bspGetNodeForCoords(const Vec3d& p);
void bspFreeMap();
int bspLoadMap(const char *fname);
void bspInit();
void bspDone();
int bspIsLoaded();

#endif /* _BSP_H */
