#ifndef _BSP_H
#define _BSP_H 1

#include <game/aabb.hh>
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

class Wall {
public:
  Wall(
    const Vec2d& a, const Vec2d& b,
    unsigned flags,
    std::tr1::shared_ptr<Sector> sectorBehind,
    const Surfaced& s
  )
  : a_(a), b_(b)
  , flags_(flags)
  , n_(norm(perpendicular(b - a)))
  , sectorBehind_(sectorBehind)
  , s_(s)
  {}

  Vec2d a() const { return a_; }
  Vec2d b() const { return b_; }
  Vec2d n() const { return n_; }
  std::tr1::shared_ptr<const Sector> sectorBehind() const { return sectorBehind_; }
  Surfaced s() const { return s_; }

  Vec2d nearestPoint(const Vec2d& pm) const;
  bool crossable() const;

private:
  Vec2d a_, b_;
  unsigned flags_;
  Vec2d n_;
  std::tr1::shared_ptr<Sector> sectorBehind_;
  Surfaced s_;
};

bool isBehind(const Vec2d& p, const Wall& l);

typedef std::vector<Wall> Walls;

class Leaf;

class Node {
public:
  Node() {}
  virtual ~Node() {}

  const Aabb3d bb() const { return bb_; }

  virtual const Leaf* findLeaf(const Vec3d& p) const = 0;
  virtual void render() const = 0;

protected:
  void addAabbPoint(const Vec3d& v) { bb_.add(v); }
  void addAabb(const Aabb3d& bb) { bb_.add(bb); }

private:
  Node(const Node&);
  Node& operator=(const Node&);

  Aabb3d bb_;
};

typedef std::vector<std::tr1::shared_ptr<Sector> > Sectors;

class Branch : public Node {
public:
  static std::auto_ptr<Node> create(std::istream& is, const Sectors& sectors);

  Branch(const Plane2d& div, std::auto_ptr<Node>& front, std::auto_ptr<Node>& back);

  Plane2d div() const { return div_; }
  const std::auto_ptr<Node>& front() const { return front_; }
  const std::auto_ptr<Node>& back() const { return back_; }

  virtual const Leaf* findLeaf(const Vec3d& p) const;
  virtual void render() const;

private:
  Plane2d div_;
  std::auto_ptr<Node> front_, back_;
};

class Leaf : public Node {
public:
  static std::auto_ptr<Node> create(std::istream& is, const Sectors& sectors);

  Leaf(const std::tr1::shared_ptr<Sector>& s) : s_(s) {}

  typedef Walls::const_iterator const_iterator;
  typedef Walls::const_reverse_iterator const_reverse_iterator;

  const_iterator begin() const { return ls_.begin(); }
  const_iterator end() const { return ls_.end(); }
  const_reverse_iterator rbegin() const { return ls_.rbegin(); }
  const_reverse_iterator rend() const { return ls_.rend(); }

  std::tr1::shared_ptr<const Sector> s() const { return s_; }

  void collide(MassPoint3d& mp) const;
  virtual const Leaf* findLeaf(const Vec3d& p) const;
  virtual void render() const;
  void reserve(size_t count) { ls_.reserve(count); }
  void add(const Wall& w);

private:
  Walls ls_;
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
