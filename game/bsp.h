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
#include <tr1/memory>

typedef Vec2d Vertex;

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
    const Vec2d& a, const Vec2d& b,
    unsigned flags,
    std::tr1::shared_ptr<Sector> sectorBehind,
    const Surfaced& s
  )
  : a_(a), b_(b)
  , flags_(flags)
  , sectorBehind_(sectorBehind)
  , s_(s)
  {}

  Vec2d a() const { return a_; }
  Vec2d b() const { return b_; }
  Vec2d n() const { return n_; }
  std::tr1::shared_ptr<const Sector> sectorBehind() const { return sectorBehind_; }
  Surfaced s() const { return s_; }

  void n(const Vec2d& value) { n_ = value; }

  Vec2d nearestPoint(const Vec2d& pm) const;
  bool crossable() const;

private:
  Vec2d a_, b_;
  unsigned flags_;
  Vec2d n_;
  std::tr1::shared_ptr<Sector> sectorBehind_;
  Surfaced s_;
};

bool isBehind(const Vec2d& p, const Line& l);

typedef std::vector<Line> Lines;

class Leaf;

class Node {
public:
  Node() : front_(0), back_(0) {}
  virtual ~Node() {}

  Plane2d div() const { return div_; }
  const std::auto_ptr<Node>& front() const { return front_; }
  const std::auto_ptr<Node>& back() const { return back_; }
  BBox3d bb() const { return bb_; }
  BBox3d& bb() { return bb_; }

  void div(const Plane2d& p) { div_ = p; }
  void front(std::auto_ptr<Node> value) { front_ = value; }
  void back(std::auto_ptr<Node> value) { back_ = value; }
  void bb(const BBox3d& value) { bb_ = value; }

  virtual const Leaf* findLeaf(const Vec3d& p) const;

private:
  Node(const Node&);
  Node& operator=(const Node&);

  BBox3d bb_;
  std::auto_ptr<Node> front_, back_;
  Plane2d div_;
};

typedef std::vector<std::tr1::shared_ptr<Sector> > Sectors;

class Leaf : public Node {
public:
  static std::auto_ptr<Node> create(std::istream& is, const Sectors& sectors);

  Leaf(const std::tr1::shared_ptr<Sector>& s) : s_(s) {}

  typedef Lines::const_iterator const_iterator;
  typedef Lines::const_reverse_iterator const_reverse_iterator;

  const_iterator begin() const { return ls_.begin(); }
  const_iterator end() const { return ls_.end(); }
  const_reverse_iterator rbegin() const { return ls_.rbegin(); }
  const_reverse_iterator rend() const { return ls_.rend(); }

  std::tr1::shared_ptr<const Sector> s() const { return s_; }
  bool empty() const { return ls_.empty(); }

  Lines& ls() { return ls_; }
  const Lines& ls() const { return ls_; }

  void collide(MassPoint3d& mp) const;
  virtual const Leaf* findLeaf(const Vec3d& p) const;

private:
  Lines ls_;
  std::tr1::shared_ptr<Sector> s_;
};

extern std::auto_ptr<Node> root;

void bspCollideTree(MassPoint3d& mp);
const Leaf* bspGetLeafForCoords(const Vec3d& p);
void bspFreeMap();
int bspLoadMap(const char *fname);
void bspInit();
void bspDone();
int bspIsLoaded();

#endif /* _BSP_H */
