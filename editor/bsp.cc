/* vim: set ts=2 sw=8 tw=0 et :*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL/SDL.h>
#include "bsp.h"
#include "ed.h"
#include "gr.h"
#include "line.h"

#include <vector>
#include <cassert>
#include <stdexcept>
#include <memory>
#include <algorithm>

namespace bsp {

struct Vertex {
  int x, y;
  int s;

  Vertex() : x(0), y(0), s(0) {}
  Vertex(int x0, int y0) : x(x0), y(y0), s(0) {}
};

class Plane2d {
public:
  Plane2d() : a_(), b_(), c_() {}
  Plane2d(const Vertex& v1, const Vertex& v2)
  : a_(v2.y - v1.y)
  , b_(v1.x - v2.x)
  , c_(-(a_ * v1.x + b_ * v1.y))
  {}

  int determine(const Vertex& v) const { return a_ * v.x + b_ * v.y + c_; }
  int dot(int dx, int dy) const { return a_ * dy - b_ * dx; }
  void save(FILE* fp) const;
  void print() const;
  friend Vertex intersect(const Plane2d& lhs, const Plane2d& rhs);

private:
  int a_, b_, c_;
};

void
Plane2d::save(FILE* fp)
const
{
  fwrite(&a_, sizeof(a_), 1, fp);
  fwrite(&b_, sizeof(b_), 1, fp);
  fwrite(&c_, sizeof(c_), 1, fp);
}

void
Plane2d::print()
const
{
	printf("Plane2d a=%d b=%d c=%d\n", a_, b_, c_);
}

Vertex
intersect(const Plane2d& lhs, const Plane2d& rhs)
{
  const int q = rhs.a_ * lhs.b_ - lhs.a_ * rhs.b_;
  if (!q) throw std::overflow_error("parallel planes do not intersect each other");
  const int x = round(float(rhs.b_ * lhs.c_ - lhs.b_ * rhs.c_) / q);
  const int y = round(float(lhs.a_ * rhs.c_ - rhs.a_ * lhs.c_) / q);
  return Vertex(x, y);
}

struct Node;

struct Line {
  int a, b;
  int c, l, r, n;
  int pd;
  int u1, u2, v;
  unsigned flags;
  Node* neigh;
  int t;
  int s;

  Line() : a(0), b(0), c(0), l(0), r(0), n(0), pd(0), u1(0), u2(0), v(0), flags(0), neigh(0), t(0), s(0) {}
  Line(int a0, int b0, int u10, int u20, int v0, unsigned flags0, int t0, int s0)
  : a(a0), b(b0), c(0), l(0), r(0), n(0), pd(0), u1(u10), u2(u20), v(v0), flags(flags0), neigh(0), t(t0), s(s0)
  {}
};

typedef std::vector<Line> Lines;

struct Node {
  Lines p;
  int s;
  std::auto_ptr<Node> l, r;
  Plane2d div;

  Node() : s(0) {}

  std::auto_ptr<Node> duplicate() const;
  void show() const;
  size_t countNodes() const;
  size_t countLines() const;
  bool isConnected(int a, int b) const;
  bool empty() const { return p.empty() && !l.get() && !r.get(); }
  int aLine(int a) const;
  int bLine(int b) const;

private:
  Node(const Node&);
  Node& operator=(const Node&);
};

struct Sector {
  int s;
  Node* n;

  Sector() : s(0), n(0) {}
  Sector(int s0, Node* n0) : s(s0), n(n0) {}
};

typedef std::vector<Vertex> Vertexes;
typedef std::vector<Sector> Sectors;

Vertexes vc;
Sectors sc;

static std::auto_ptr<Node> root;

static int bspGetVertex(int x, int y) {
  for (unsigned i = 0; i < vc.size(); ++i)
    if (vc[i].x == x && vc[i].y == y)
      return i;
  return -1;
}

static int bspAddVertex(int x, int y) {
  {
    int i = bspGetVertex(x, y);
    if (i >= 0) return i;
  }
  vc.push_back(Vertex(x, y));
  return vc.size() - 1;
}

static int
bspAddSector()
{
  sc.push_back(Sector());
  return sc.size() - 1;
}

std::auto_ptr<Node>
Node::duplicate()
const
{
  std::auto_ptr<Node> p(new Node());
  if (!l.get() && !r.get()) {
    int i = bspAddSector();
    if (i < 0) return p;
    sc[i].n = p.get();
  }
  if (l.get()) p->l = l->duplicate();
  if (r.get()) p->r = r->duplicate();
  return p;
}

static Node*
bspGetNodeForSector(int s)
{
  if (!s) return NULL;
  for (unsigned i = 0; i < sc.size(); ++i)
    if (sc[i].s == s)
      return sc[i].n;
  for (unsigned i = 0; i < sc.size(); ++i)
    if (!sc[i].s) {
      sc[i].s = sc[i].n->s = s;
      return sc[i].n;
    }
  std::auto_ptr<Node> p(new Node());
  p->l.reset(root.release());
  p->r = p->l->duplicate();
  root.reset(p.release());
  return bspGetNodeForSector(s);
}

int bspAddLine(int s, int, int x1, int y1, int x2, int y2, int u, int v, int flags, int t, int du) {
  int a = bspAddVertex(x1, y1);
  int b = bspAddVertex(x2, y2);
  Node* n = bspGetNodeForSector(s);
  if (n == NULL) return -1;
  if (a < 0 || b < 0) return -1;
  Line line(a, b, u + sqrtf((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1)) + du, u, v, flags, t, s);
  for (unsigned i = 0; i < sc.size(); ++i) {
    if (sc[i].s != s) {
      Node* p = sc[i].n;
      for (unsigned j = 0; j < p->p.size(); ++j) {
        if (p->p[j].a == b && p->p[j].b == a) {
          line.neigh = p;
          p->p[j].neigh = n;
          break;
        }
      }
      if (line.neigh) break;
    }
  }
  n->p.push_back(line);
  return n->p.size() - 1;
}

void
Node::show()
const
{
  if (l.get()) l->show();
  if (r.get()) r->show();
  if (!p.empty()) {
    grSetColor((s * 343) | 3);
    for (unsigned i = 0; i < p.size(); ++i)
      edVector(vc[p[i].a].x, vc[p[i].a].y, vc[p[i].b].x, vc[p[i].b].y);
  }
}

void bspShow() {
  if (root.get()) root->show();
  grSetColor(255);
  for (unsigned i = 0; i < vc.size(); ++i) edVertex(vc[i].x, vc[i].y);
}

size_t
Node::countNodes()
const
{
  size_t result = 1;
  if (l.get()) result += l->countNodes();
  if (r.get()) result += r->countNodes();
  return result;
}

size_t
Node::countLines()
const
{
  size_t result = p.size();
  if (l.get()) result += l->countLines();
  if (r.get()) result += r->countLines();
  return result;
}

struct VertexLessX : public std::binary_function<unsigned, unsigned, bool> {
  bool operator()(unsigned lhs, unsigned rhs) const { return vc[rhs].x < vc[lhs].x; }
};

struct VertexLessY : public std::binary_function<unsigned, unsigned, bool> {
  bool operator()(unsigned lhs, unsigned rhs) const { return vc[rhs].y < vc[lhs].y; }
};

template <class Iterator>
static void
bspSortVerteces(Iterator first, Iterator last)
{
  if (first == last || first + 1 == last) return;
  Iterator second = first + 1;
  if (abs(vc[*second].x - vc[*first].x) > abs(vc[*second].y - vc[*first].y)) {
    std::sort(first, last, VertexLessX());
  } else {
    std::sort(first, last, VertexLessY());
  }
}

bool
Node::isConnected(int a, int b)
const
{
  for (Lines::const_iterator i(p.begin()); i != p.end(); ++i) {
    if (i->a == a && i->b == b) return true;
  }
  return false;
}

int
Node::aLine(int a)
const
{
  unsigned j = 0;
  int bo;
  for (unsigned i = 0; i < p.size(); ++i)
    if (p[i].a == a) {
      bo = 0;
      for (; j < p.size(); ++j)
        if (p[j].b == a) {
          ++j;
          ++bo;
          break;
        }
      if (!bo) return s;
    }
  return 0;
}

int
Node::bLine(int b)
const
{
  unsigned j = 0;
  int bo;
  for (unsigned i = 0; i < p.size(); ++i)
    if (p[i].b == b) {
      bo = 0;
      for (; j < p.size(); ++j)
        if (p[j].a == b) {
          ++j;
          ++bo;
          break;
        }
      if (!bo) return s;
    }
  return 0;
}

static int bspGetPair(Node* n, unsigned l) {
  Node* p = n->p[l].neigh;
  unsigned i;
  for (i = 0; i < p->p.size(); ++i)
    if (p->p[i].a == n->p[l].b && p->p[i].b == n->p[l].a) {
      if (p->p[i].neigh != n) printf("pair neighje mashova mutat\n");
      return i;
    }
  return -1;
}

static void bspNoticePair(Node* n, unsigned l, Node* p) {
  int i = bspGetPair(n, l);
  if (i != -1) n->p[l].neigh->p[i].neigh = p;
}

static void
bspCleanSub(std::auto_ptr<Node>& n)
{
  if (n->l.get()) bspCleanSub(n->l);
  if (n->r.get()) bspCleanSub(n->r);
  if (n->empty()) n.reset();
}

void bspCleanTree() {
  if (root.get()) bspCleanSub(root);
}

//int bspLoad(FILE *f);

static void
bspBuildSub(Node* n)
{
  if (n == NULL || n->p.size() < 3) return;
//  if (n->s != 31) return;
//  printf("bspBuildSub(): %d\n", n->n);
  for (unsigned i = 0; i < n->p.size(); ++i) {
    n->p[i].l = n->p[i].r = n->p[i].n = 0;
    const Plane2d div(vc[n->p[i].a], vc[n->p[i].b]);
    for (unsigned j = 0; j < n->p.size(); ++j) {
      const int da = div.determine(vc[n->p[j].a]);
      const int db = div.determine(vc[n->p[j].b]);
      if ((da < 0 && db > 0) || (da > 0 && db < 0)) {
        ++n->p[i].n;
        continue;
      }
      if (da < 0) ++n->p[i].r; else if (da > 0) ++n->p[i].l;
      if (db < 0) ++n->p[i].r; else if (db > 0) ++n->p[i].l;
    }
  }
  int j = 0;
  for (unsigned i = 1; i < n->p.size(); ++i) {
//    printf("nodeline candidate: %d r=%d l=%d\n", i, n->p[i].r, n->p[i].l);
    if (!n->p[j].r || !n->p[j].l) {
      j = i;
      continue;
    }
    const int da = abs(n->p[i].r - n->p[i].l);
    const int db = abs(n->p[j].r - n->p[j].l);
    if ((n->p[i].l || n->p[i].n) && ((da < db) || ((da == db) && (n->p[i].n < n->p[j].n)))) j = i;
  }
//  printf("nodeline: %d r=%d l=%d\n", j, n->p[j].r, n->p[j].l);
  n->div = Plane2d(vc[n->p[j].a], vc[n->p[j].b]);
  printf("%p\n", static_cast<void*>(n));
  n->div.print();
  for (unsigned i = 0; i < vc.size(); ++i) vc[i].s = 0;
  int mina = 0;
  int maxa = 0;
  int e = 0;
  int f = 0;
  {
    int l = 0;
    int r = 0;
    for (unsigned i = 0; i < n->p.size(); ++i) {
      n->p[i].l = n->p[i].r = 0;
      int da = n->div.determine(vc[n->p[i].a]);
      const int db = n->div.determine(vc[n->p[i].b]);
      if (da < mina) mina = da;
      if (db < mina) mina = db;
      if (da > maxa) maxa = da;
      if (db > maxa) maxa = db;
      if (!da && !vc[n->p[i].a].s) {
        ++f;
        ++vc[n->p[i].a].s;
      }
      if (!db && !vc[n->p[i].b].s) {
        ++f;
        ++vc[n->p[i].b].s;
      }
      if (!da && !db) {
        const int dx2 = vc[n->p[i].a].x - vc[n->p[i].b].x;
        const int dy2 = vc[n->p[i].a].y - vc[n->p[i].b].y;
        da = n->div.dot(dx2, dy2);
      }
      if ((da < 0 && db <= 0) || (da <= 0 && db < 0)) {
        ++n->p[i].r;
        ++r;
        continue;
      }
      if ((da > 0 && db >= 0) || (da >= 0 && db > 0)) {
        ++n->p[i].l;
        ++l;
        continue;
      }
      ++e;
    }
    if (!l && !e) return;

    f += e;
  /* here f tells us how many verteces are on the nodeline */
//  printf("f=%d mina=%d maxa=%d\n", f, mina, maxa);
    if (mina >= 10 || maxa <= 10) return;

    n->l.reset(new Node());
    n->l->p.reserve(l + e + f);

    n->r.reset(new Node());
    n->r->p.reserve(r + e + f);
  }

  n->r->s = n->l->s = n->s;

  Line tl;
  for (unsigned i = 0; i < n->p.size(); ++i) {
    if (n->p[i].r) {
      n->r->p.push_back(n->p[i]);
      if (n->p[i].neigh) bspNoticePair(n, i, n->r.get());
      continue;
    }
    if (n->p[i].l) {
      n->l->p.push_back(n->p[i]);
      if (n->p[i].neigh) bspNoticePair(n, i, n->l.get());
      continue;
    }
    const int da = n->div.determine(vc[n->p[i].a]);
    const Vertex v(intersect(n->div, Plane2d(vc[n->p[i].a], vc[n->p[i].b])));
    if ((e = bspGetVertex(v.x, v.y)) == -1) e = bspAddVertex(v.x, v.y);
    vc[e].s = 1;
    Node* const ne = n->p[i].neigh;
    int t = (ne != NULL) ? bspGetPair(n, i) : -1;
    if (t >= 0) {
      tl = ne->p[t];
      ne->p[t] = ne->p.back();
      ne->p.pop_back();
    }
    t = t != -1;
    if (da > 0) {
      int dx2 = 0;
      int dy2 = 0;
      if (abs(vc[n->p[i].b].x - vc[n->p[i].a].x) > abs(vc[n->p[i].b].y - vc[n->p[i].a].y)) {
        dx2 = (n->p[i].u1 - n->p[i].u2) * (vc[n->p[i].b].x - v.x);
        dx2 = n->p[i].u2 + (dx2 / (vc[n->p[i].b].x - vc[n->p[i].a].x));
        if (t) {
          dy2 = (tl.u1 - tl.u2) * (v.x - vc[tl.b].x);
          dy2 = tl.u2 + (dy2 / (vc[tl.a].x - vc[tl.b].x));
        }
      } else {
        dx2 = (n->p[i].u1 - n->p[i].u2) * (vc[n->p[i].b].y - v.y);
        dx2 = n->p[i].u2 + (dx2 / (vc[n->p[i].b].y - vc[n->p[i].a].y));
        if (t) {
          dy2 = (tl.u1 - tl.u2) * (v.y - vc[tl.b].y);
          dy2 = tl.u2 + (dy2 / (vc[tl.a].y - vc[tl.b].y));
        }
      }
      if (n->p[i].a != e) {
        Line line2(n->p[i].a, e, n->p[i].u1, dx2, n->p[i].v, 0, n->p[i].t, n->p[i].s);
        if (t) {
          line2.neigh = ne;
          Line line(e, tl.b, dy2, tl.u2, tl.v, 0, tl.t, tl.s);
          line.neigh = n->l.get();
          ne->p.push_back(line);
        }
        n->l->p.push_back(line2);
      }
      if (n->p[i].b != e) {
        Line line2(e, n->p[i].b, dx2, n->p[i].u2, n->p[i].v, 0, n->p[i].t, n->p[i].s);
        if (t) {
          line2.neigh = ne;
          Line line(tl.a, e, tl.u1, dy2, tl.v, 0, tl.t, tl.s);
          line.neigh = n->r.get();
          ne->p.push_back(line);
        }
        n->r->p.push_back(line2);
      }
    } else {
      int dx2 = 0;
      int dy2 = 0;
      if (abs(vc[n->p[i].b].x - vc[n->p[i].a].x) > abs(vc[n->p[i].b].y - vc[n->p[i].a].y)) {
        dx2 = (n->p[i].u1 - n->p[i].u2) * (vc[n->p[i].b].x - v.x);
        dx2 = n->p[i].u2 + (dx2 / (vc[n->p[i].b].x - vc[n->p[i].a].x));
        if (t) {
          dy2 = (tl.u1 - tl.u2) * (v.x - vc[tl.b].x);
          dy2 = tl.u2 + (dy2 / (vc[tl.a].x - vc[tl.b].x));
        }
      } else {
        dx2 = (n->p[i].u1 - n->p[i].u2) * (vc[n->p[i].b].y - v.y);
        dx2 = n->p[i].u2 + (dx2 / (vc[n->p[i].b].y - vc[n->p[i].a].y));
        if (t) {
          dy2 = (tl.u1 - tl.u2) * (v.y - vc[tl.b].y);
          dy2 = tl.u2 + (dy2 / (vc[tl.a].y - vc[tl.b].y));
        }
      }
      if (n->p[i].a != e) {
        Line line2(n->p[i].a, e, n->p[i].u1, dx2, n->p[i].v, 0, n->p[i].t, n->p[i].s);
        if (t) {
          line2.neigh = ne;
          Line line(e, tl.b, dy2, tl.u2, tl.v, 0, tl.t, tl.s);
          line.neigh = n->r.get();
          ne->p.push_back(line);
        }
        n->r->p.push_back(line2);
      }
      if (n->p[i].b != e) {
        Line line2(e, n->p[i].b, dx2, n->p[i].u2, n->p[i].v, 0, n->p[i].t, n->p[i].s);
        if (t) {
          line2.neigh = ne;
          Line line(tl.a, e, tl.u1, dy2, tl.v, 0, tl.t, tl.s);
          line.neigh = n->l.get();
          ne->p.push_back(line);
        }
        n->l->p.push_back(line2);
      }
    }
  }

  n->p.clear();

  std::vector<unsigned> p(f);
  int t = 0;
  for (unsigned i = 0; i < vc.size(); ++i) if (vc[i].s) p[t++] = i;
  bspSortVerteces(p.begin(), p.end());
  const int dx2 = vc[p[0]].x - vc[p[t-1]].x;
  const int dy2 = vc[p[0]].y - vc[p[t-1]].y;
  --t;
  {
    const bool det = n->div.dot(dx2, dy2) > 0;
    Node* const na = (det ? n->r : n->l).get();
    Node* const nb = (det ? n->l : n->r).get();
    for (int i = 0; i < t; ++i) {
      const bool ba = na->aLine(p[i]) && !na->isConnected(p[i + 1], p[i]);
      const bool bb = nb->bLine(p[i]) && !nb->isConnected(p[i], p[i + 1]);
      if (ba) {
        Line line(p[i + 1], p[i], 0, 0, 0, ::Line::Flag::NOTHING, 0, na->s);
        na->p.push_back(line);
      }
      if (bb) {
        Line line(p[i], p[i + 1], 0, 0, 0, ::Line::Flag::NOTHING, 0, nb->s);
        nb->p.push_back(line);
        if (ba) {
          na->p.back().neigh = nb;
          nb->p.back().neigh = na;
        }
      }
    }
  }

  e = rand() | 3;
  for (unsigned i = 0; i < n->r->p.size(); ++i) n->r->p[i].c = e;
  e = rand() | 3;
  for (unsigned i = 0; i < n->l->p.size(); ++i) n->l->p[i].c = e;

  grBegin();
  n->show();
  grEnd();
//  SDL_Delay(10);
  bspBuildSub(n->l.get());
  bspBuildSub(n->r.get());
  n->p.clear();
}

static void
bspBuildSearch(Node* n)
{
  printf("zizi: %p\n", static_cast<void*>(n));
  if (n->p.empty()) {
    if (n->l.get()) bspBuildSearch(n->l.get());
    if (n->r.get()) bspBuildSearch(n->r.get());
  } else {
    int e = rand() | 3;
    for (unsigned i = 0; i < n->p.size(); ++i) n->p[i].c = e;
    bspBuildSub(n);
  }
}

void bspBuildTree() {
  if (!root.get()) {
    printf("no tree to build\n");
    return;
  }
  bspCleanTree();
  printf("bspBuildTree():\n vertex: %lu\n", vc.size());
  bspBuildSearch(root.get());
  printf("bspBuildTree(): done\n");
  printf("nodes:%lu lines:%lu\n", root->countNodes(), root->countLines());
/*  FILE *fp = fopen("map.bsp", "rb");
  bspLoad(fp);
  fclose(fp);*/
}

int bspInit() {
  vc.clear();
  sc.clear();

  root.reset(new Node());

  sc.push_back(Sector(0, root.get()));

  return !0;
}

void bspDone() {
  root.reset();
  sc.clear();
  vc.clear();
}

static void
bspPrintTreeSub(Node* n, unsigned d = 0)
{
  for (unsigned i = 0; i < d; ++i) putchar(' ');
  printf("%lu\n", n->p.size());
  ++d;
  if (n->l.get()) bspPrintTreeSub(n->l.get(), d);
  if (n->r.get()) bspPrintTreeSub(n->r.get(), d);
}

void bspPrintTree() {
  if (root.get()) bspPrintTreeSub(root.get());
}

static int
bspSaveVertex(FILE *fp, Vertex *v)
{
  unsigned char buf[2 * 2], *p = buf;
  *(short *)p = v->x;
  *(short *)(p + 2) = v->y;
  return (fwrite(buf, 1, sizeof(buf), fp) == sizeof(buf)) ? 0 : -1;
}

static int
bspSaveLine(FILE* fp, Line* l)
{
  unsigned char buf[2 * 2 + 1 + 3 * 2 + 4], *p = buf;
  *(short *)p = l->a;
  *(short *)(p + 2) = l->b;
  p[4] = l->neigh ? ::Line::Flag::TWOSIDED : ::Line::Flag::NOTHING;
  *(unsigned short *)(p + 5) = l->u1;
  *(unsigned short *)(p + 7) = l->u2;
  *(unsigned short *)(p + 9) = l->v;
  *(unsigned *)(p + 11) = l->t;
  return (fwrite(buf, 1, sizeof(buf), fp) == sizeof(buf)) ? 0 : -1;
}

static void
bspSaveSub(FILE* fp, Node* n)
{
  if (n == NULL) {
    unsigned i = 0;
    fwrite(&i, sizeof(unsigned), 1, fp);
    return;
  }
  {
    unsigned i = n->p.size() + 1;
    fwrite(&i, sizeof(int), 1, fp);
  }
  n->div.save(fp);
  fwrite(&n->s, sizeof(int), 1, fp);
  if (!n->p.empty()) {
    int j = n->p[0].a;
    for (unsigned i = 0; i < n->p.size(); ++i) n->p[i].n = 0;
    unsigned k = 0;
    int last_good = -1;
    for (unsigned i = 0; i < n->p.size(); ++i) {
      if (n->p[i].a == j && !n->p[i].n) {
        bspSaveLine(fp, &n->p.at(i));
        last_good = i;
        j = n->p[i].b;
        ++n->p[i].n;
        i = 0;
        ++k;
      }
    }
    if (k != n->p.size()) {
      printf("open sector!!! (%d) k=%d n->n=%lu\n", n->s, k, n->p.size());
      for (unsigned i = 0; i < n->p.size(); ++i)
        printf(" %d %d\n", n->p[i].a, n->p[i].b);
      // halvany lila workaround ...
      for (; k < n->p.size(); ++k) {
        assert(last_good >= 0);
        bspSaveLine(fp, &n->p.at(last_good));
      }
    }
  }
  bspSaveSub(fp, n->l.get());
  bspSaveSub(fp, n->r.get());
}

int bspSave(FILE *f) {
  const unsigned vertexCount = vc.size();
  fwrite(&vertexCount, sizeof(unsigned), 1, f);
  for (unsigned i = 0; i < vc.size(); ++i) {
    bspSaveVertex(f, &vc.at(i));
  }
//  bspPrintTree();
  unsigned nodeCount = root->countNodes();
  unsigned lineCount = root->countLines();
  fwrite(&nodeCount, sizeof(unsigned), 1, f);
  fwrite(&lineCount, sizeof(unsigned), 1, f);
  bspSaveSub(f, root.get());
  return !0;
}

} // namespace bsp
