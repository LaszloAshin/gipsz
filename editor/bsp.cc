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

namespace bsp {

struct Vertex {
  int x, y;
  int s;

  Vertex() : x(0), y(0), s(0) {}
  Vertex(int x0, int y0) : x(x0), y(y0), s(0) {}
};

struct Node;

struct Line {
  int a, b;
  int c, l, r, n;
  int pd;
  int u1, u2, v;
  unsigned flags;
  Node* neigh;
  int t;

  Line() : a(0), b(0), c(0), l(0), r(0), n(0), pd(0), u1(0), u2(0), v(0), flags(0), neigh(0), t(0) {}
  Line(int a0, int b0, int u10, int u20, int v0, unsigned flags0, int t0)
  : a(a0), b(b0), c(0), l(0), r(0), n(0), pd(0), u1(u10), u2(u20), v(v0), flags(flags0), neigh(0), t(t0)
  {}
};

typedef std::vector<Line> Lines;

struct Node {
  Lines p;
  int s;
  Node* l;
  Node* r;

  Node() : s(0), l(0), r(0) {}

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

static Node* root = 0;

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

static void
bspDuplicateNode(Node* n, Node** p) // XXX: reference
{
  *p = new Node();
  if (!n->l && !n->r) {
    int i = bspAddSector();
    if (i < 0) return;
    sc[i].n = *p;
  }
  if (n->l) bspDuplicateNode(n->l, &(*p)->l);
  if (n->r) bspDuplicateNode(n->r, &(*p)->r);
}

static void
bspDuplicateTree(Node* n)
{
  bspDuplicateNode(n->l, &n->r);
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
  Node* p = new Node();
  p->l = root;
  bspDuplicateTree(p);
  root = p;
  return bspGetNodeForSector(s);
}

int bspAddLine(int s, int x1, int y1, int x2, int y2, int u, int v, int flags, int t, int du) {
  int a = bspAddVertex(x1, y1);
  int b = bspAddVertex(x2, y2);
  Node* n = bspGetNodeForSector(s);
  if (n == NULL) return -1;
  if (a < 0 || b < 0) return -1;
  Line line(a, b, u + sqrtf((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1)) + du, u, v, flags, t);
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

static bool
bspIntersect(const Vertex& p1, const Vertex& p2, const Vertex& p3, const Vertex& p4, Vertex& mp)
{
  const double dx21 = p2.x - p1.x;
  const double dx43 = p4.x - p3.x;
  const double dy21 = p2.y - p1.y;
  const double dy43 = p4.y - p3.y;
  const double dy13 = p1.y - p3.y;
  const double t = dy43 * dx21 - dy21 * dx43;
  if (!t) return false; /* parallel */
  double y;
  const double x = (dx21 * (dy13 * dx43 + dy43 * p3.x) - (dy21 * dx43 * p1.x)) / t;
  if (abs(dx21) > abs(dx43)) {
    if (dx21) {
      y = p1.y + (x - p1.x) * dy21 / dx21;
    } else {
      return false;
    }
  } else if (dx43) {
    y = p3.y + (x - p3.x) * dy43 / dx43;
  } else {
    return false;
  }
  mp.x = round(x);
  mp.y = round(y);
  return true;
}

static void bspShowSub(Node* n) {
  if (n->l != NULL) bspShowSub(n->l);
  if (n->r != NULL) bspShowSub(n->r);
  if (!n->p.empty()) {
    grSetColor((n->s * 343) | 3);
    for (unsigned i = 0; i < n->p.size(); ++i)
      edVector(vc[n->p[i].a].x, vc[n->p[i].a].y, vc[n->p[i].b].x, vc[n->p[i].b].y);
  }
}

void bspShow() {
  if (root != NULL) bspShowSub(root);
  grSetColor(255);
  for (unsigned i = 0; i < vc.size(); ++i) edVertex(vc[i].x, vc[i].y);
}

struct bsp_count_result {
  unsigned node_count;
  unsigned line_count;
};

static void
bspCountSub(Node* n, struct bsp_count_result *result)
{
  ++result->node_count;
  result->line_count += n->p.size();
  if (n->l != NULL) bspCountSub(n->l, result);
  if (n->r != NULL) bspCountSub(n->r, result);
}

static void
bspCount(struct bsp_count_result *result)
{
  result->node_count = 0;
  result->line_count = 0;
  if (root != NULL) bspCountSub(root, result);
}

static void bspSortVerteces(unsigned *p, unsigned n) {
  if (n < 2) return;
  unsigned i, j, k;
  if (abs(vc[p[1]].x - vc[p[0]].x) > abs(vc[p[1]].y - vc[p[0]].y)) {
    for (i = 0; i < n; ++i) {
      k = i;
      for (j = i + 1; j < n; ++j)
        if (vc[p[j]].x > vc[p[k]].x) k = j;
      if (k != i) {
        j = p[k];
        p[k] = p[i];
        p[i] = j;
      }
    }
  } else {
    for (i = 0; i < n; ++i) {
      k = i;
      for (j = i + 1; j < n; ++j)
        if (vc[p[j]].y > vc[p[k]].y) k = j;
      if (k != i) {
        j = p[k];
        p[k] = p[i];
        p[i] = j;
      }
    }
  }
}

static int bspMayConnect(Node* n, int a, int b) {
  for (unsigned i = 0; i < n->p.size(); ++i) {
    if ((n->p[i].a == a && n->p[i].b == b))
      return 0;
    n->p[i].n = 0;
  }
  return !0;
}

static int bspALine(Node* n, int a) {
  unsigned j = 0;
  int bo;
  for (unsigned i = 0; i < n->p.size(); ++i)
    if (n->p[i].a == a) {
      bo = 0;
      for (; j < n->p.size(); ++j)
        if (n->p[j].b == a) {
          ++j;
          ++bo;
          break;
        }
      if (!bo) return n->s;
    }
  return 0;
}

static int bspBLine(Node* n, int b) {
  unsigned j = 0;
  int bo;
  for (unsigned i = 0; i < n->p.size(); ++i)
    if (n->p[i].b == b) {
      bo = 0;
      for (; j < n->p.size(); ++j)
        if (n->p[j].a == b) {
          ++j;
          ++bo;
          break;
        }
      if (!bo) return n->s;
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
bspCleanSub(Node** n)
{
  if ((*n)->l != NULL) bspCleanSub(&(*n)->l);
  if ((*n)->r != NULL) bspCleanSub(&(*n)->r);
  if ((*n)->l == NULL && (*n)->r == NULL && (*n)->p.empty()) {
    delete *n;
    *n = 0;
  }
}

void bspCleanTree() {
  if (root != NULL) bspCleanSub(&root);
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
    const int dx1 = vc[n->p[i].b].x - vc[n->p[i].a].x;
    const int dy1 = vc[n->p[i].b].y - vc[n->p[i].a].y;
    for (unsigned j = 0; j < n->p.size(); ++j) {
      const int dx2a = vc[n->p[j].a].x - vc[n->p[i].a].x;
      const int dy2a = vc[n->p[j].a].y - vc[n->p[i].a].y;
      const int da = dy1 * dx2a - dy2a * dx1;
      const int dx2b = vc[n->p[j].b].x - vc[n->p[i].a].x;
      const int dy2b = vc[n->p[j].b].y - vc[n->p[i].a].y;
      const int db = dy1 * dx2b - dy2b * dx1;
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
  const int dx1 = vc[n->p[j].b].x - vc[n->p[j].a].x;
  const int dy1 = vc[n->p[j].b].y - vc[n->p[j].a].y;
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
      const int dx2a = vc[n->p[i].a].x - vc[n->p[j].a].x;
      const int dy2a = vc[n->p[i].a].y - vc[n->p[j].a].y;
      int da = dy1 * dx2a - dy2a * dx1;
      const int dx2b = vc[n->p[i].b].x - vc[n->p[j].a].x;
      const int dy2b = vc[n->p[i].b].y - vc[n->p[j].a].y;
      const int db = dy1 * dx2b - dy2b * dx1;
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
        da = dy1 * dy2 + dx1 * dx2;
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

    n->l = new Node();
    n->l->p.reserve(l + e + f);

    n->r = new Node();
    n->r->p.reserve(r + e + f);
  }

  n->r->s = n->l->s = n->s;

  Line tl;
  for (unsigned i = 0; i < n->p.size(); ++i) {
    if (n->p[i].r) {
      n->r->p.push_back(n->p[i]);
      if (n->p[i].neigh != NULL) bspNoticePair(n, i, n->r);
      continue;
    }
    if (n->p[i].l) {
      n->l->p.push_back(n->p[i]);
      if (n->p[i].neigh != NULL) bspNoticePair(n, i, n->l);
      continue;
    }
    const int dx2a = vc[n->p[i].a].x - vc[n->p[j].a].x;
    const int dy2a = vc[n->p[i].a].y - vc[n->p[j].a].y;
    const int da = dy1 * dx2a - dy2a * dx1;
    Vertex v;
    if (!bspIntersect(vc[n->p[j].a], vc[n->p[j].b], vc[n->p[i].a], vc[n->p[i].b], v)) {
      throw std::runtime_error("no intersection kutya");
    }
    if ((e = bspGetVertex(v.x, v.y)) == -1) e = bspAddVertex(v.x, v.y);
    vc[e].s = 1;
    Node * const ne = n->p[i].neigh;
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
        Line line2(n->p[i].a, e, n->p[i].u1, dx2, n->p[i].v, 0, n->p[i].t);
        if (t) {
          line2.neigh = ne;
          Line line(e, tl.b, dy2, tl.u2, tl.v, 0, tl.t);
          line.neigh = n->l;
          ne->p.push_back(line);
        }
        n->l->p.push_back(line2);
      }
      if (n->p[i].b != e) {
        Line line2(e, n->p[i].b, dx2, n->p[i].u2, n->p[i].v, 0, n->p[i].t);
        if (t) {
          line2.neigh = ne;
          Line line(tl.a, e, tl.u1, dy2, tl.v, 0, tl.t);
          line.neigh = n->r;
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
        Line line2(n->p[i].a, e, n->p[i].u1, dx2, n->p[i].v, 0, n->p[i].t);
        if (t) {
          line2.neigh = ne;
          Line line(e, tl.b, dy2, tl.u2, tl.v, 0, tl.t);
          line.neigh = n->r;
          ne->p.push_back(line);
        }
        n->r->p.push_back(line2);
      }
      if (n->p[i].b != e) {
        Line line2(e, n->p[i].b, dx2, n->p[i].u2, n->p[i].v, 0, n->p[i].t);
        if (t) {
          line2.neigh = ne;
          Line line(tl.a, e, tl.u1, dy2, tl.v, 0, tl.t);
          line.neigh = n->l;
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
  bspSortVerteces(&p.front(), t);
  const int dx2 = vc[p[0]].x - vc[p[t-1]].x;
  const int dy2 = vc[p[0]].y - vc[p[t-1]].y;
  --t;
  {
    int i = 0;
    if (dy1 * dy2 + dx1 * dx2 > 0) {
      do {
        const int da = bspALine(n->r, p[i]);
        const int db = bspBLine(n->l, p[i]);
        ++i;
        j = 0;
        if (da && bspMayConnect(n->r, p[i], p[i-1])) {
          Line line(p[i], p[i - 1], 0, 0, 0, ::Line::Flag::NOTHING, 0);
          n->r->p.push_back(line);
          ++j;
        }
        if (db && bspMayConnect(n->l, p[i-1], p[i])) {
          Line line(p[i - 1], p[i], 0, 0, 0, ::Line::Flag::NOTHING, 0);
          if (j) {
            n->r->p.back().neigh = n->l;
            line.neigh = n->r;
          }
          n->l->p.push_back(line);
        }
      } while (i < t);
    } else {
      do {
        const int da = bspALine(n->l, p[i]);
        const int db = bspBLine(n->r, p[i]);
        ++i;
        j = 0;
        if (da && bspMayConnect(n->l, p[i], p[i-1])) {
          Line line(p[i], p[i - 1], 0, 0, 0, ::Line::Flag::NOTHING, 0);
          n->l->p.push_back(line);
          ++j;
        }
        if (db && bspMayConnect(n->r, p[i-1], p[i])) {
          Line line(p[i - 1], p[i], 0, 0, 0, ::Line::Flag::NOTHING, 0);
          if (j) {
            line.neigh = n->l;
            n->l->p.back().neigh = n->r;
          }
          n->r->p.push_back(line);
        }
      } while (i < t);
    }
  }

  e = rand() | 3;
  for (unsigned i = 0; i < n->r->p.size(); ++i) n->r->p[i].c = e;
  e = rand() | 3;
  for (unsigned i = 0; i < n->l->p.size(); ++i) n->l->p[i].c = e;

  grBegin();
  bspShowSub(n);
  grEnd();
//  SDL_Delay(100);
  bspBuildSub(n->l);
  bspBuildSub(n->r);
  n->p.clear();
}

static void
bspBuildSearch(Node* n)
{
  if (n->p.empty()) {
    if (n->l != NULL) bspBuildSearch(n->l);
    if (n->r != NULL) bspBuildSearch(n->r);
  } else {
    int e = rand() | 3;
    for (unsigned i = 0; i < n->p.size(); ++i) n->p[i].c = e;
    bspBuildSub(n);
  }
}

void bspBuildTree() {
  bspCleanTree();
  printf("bspBuildTree():\n vertex: %lu\n", vc.size());
  if (root != NULL) bspBuildSearch(root);
  printf("bspBuildTree(): done\n");
  struct bsp_count_result counts;
  bspCount(&counts);
  printf("nodes:%u lines:%u\n", counts.node_count, counts.line_count);
/*  FILE *fp = fopen("map.bsp", "rb");
  bspLoad(fp);
  fclose(fp);*/
}

int bspInit() {
  vc.clear();
  sc.clear();

  root = new Node();

  sc.push_back(Sector(0, root));

  return !0;
}

static void
bspDelNode(Node* n)
{
  if (n->l != NULL) bspDelNode(n->l);
  if (n->r != NULL) bspDelNode(n->r);
  delete n;
}

void bspDone() {
  if (root != NULL) bspDelNode(root);
  sc.clear();
  vc.clear();
}

static void
bspPrintTreeSub(Node* n, unsigned d)
{
  for (unsigned i = 0; i < d; ++i) putchar(' ');
  printf("%lu\n", n->p.size());
  ++d;
  if (n->l != NULL) bspPrintTreeSub(n->l, d);
  if (n->r != NULL) bspPrintTreeSub(n->r, d);
}

void bspPrintTree() {
  if (root != NULL) bspPrintTreeSub(root, 0);
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
  bspSaveSub(fp, n->l);
  bspSaveSub(fp, n->r);
}

int bspSave(FILE *f) {
  const unsigned vertexCount = vc.size();
  fwrite(&vertexCount, sizeof(unsigned), 1, f);
  for (unsigned i = 0; i < vc.size(); ++i) {
    bspSaveVertex(f, &vc.at(i));
  }
  struct bsp_count_result counts;
  bspCount(&counts);
//  bspPrintTree();
  fwrite(&counts.node_count, sizeof(unsigned), 1, f);
  fwrite(&counts.line_count, sizeof(unsigned), 1, f);
  bspSaveSub(f, root);
  return !0;
}

} // namespace bsp
