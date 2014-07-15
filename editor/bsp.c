#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL/SDL.h>
#include "bsp.h"
#include "ed.h"
#include "gr.h"
#include "line.h"

typedef struct {
  int x, y;
  int s;
} bspvertex_t;

struct node_s;

typedef struct line_s {
  int a, b;
  int c, l, r, n;
  int pd;
  int u1, u2, v;
  unsigned flags;
  struct node_s *neigh;
  int t;
} bspline_t;

typedef struct node_s {
  bspline_t *p;
  unsigned alloc, n;
  int s;
  struct node_s *l, *r;
} node_t;

typedef struct sector_s {
  int s;
  node_t *n;
} bspsector_t;

static struct {
  bspvertex_t *p;
  unsigned alloc, n;
} vc;

static struct {
  bspsector_t *p;
  unsigned alloc, n;
} sc;

static node_t *root = NULL;

static int bspGetVertex(int x, int y) {
  for (unsigned i = 0; i < vc.n; ++i)
    if (vc.p[i].x == x && vc.p[i].y == y)
      return i;
  return -1;
}

static int bspAddVertex(int x, int y) {
  {
    int i = bspGetVertex(x, y);
    if (i >= 0) return i;
  }
  if (vc.n == vc.alloc) {
    vc.alloc *= 2;
    bspvertex_t *p = (bspvertex_t *)malloc(vc.alloc * sizeof(bspvertex_t));
    if (p == NULL) return -1;
    for (unsigned i = 0; i < vc.n; ++i) p[i] = vc.p[i];
    free(vc.p);
    vc.p = p;
  }
  vc.p[vc.n].x = x;
  vc.p[vc.n].y = y;
  return vc.n++;
}

static int bspAddSector() {
  if (sc.n == sc.alloc) {
    sc.alloc *= 2;
    bspsector_t *p = (bspsector_t *)malloc(sc.alloc * sizeof(bspsector_t));
    if (p == NULL) return -1;
    for (unsigned i = 0; i < sc.n; ++i) p[i] = sc.p[i];
    free(sc.p);
    sc.p = p;
  }
  sc.p[sc.n].s = 0;
  sc.p[sc.n].n = NULL;
  return sc.n++;
}

static void
bspDuplicateNode(node_t *n, node_t **p)
{
  *p = (node_t *)malloc(sizeof(node_t));
  if (*p == NULL) return;
  if (n->l == NULL && n->r == NULL) {
    int i;
    if ((i = bspAddSector()) < 0) return;
    sc.p[i].n = *p;
  }
  (*p)->alloc = (*p)->n = 0;
  (*p)->p = NULL;
  (*p)->s = 0;
  if (n->l != NULL) bspDuplicateNode(n->l, &(*p)->l); else (*p)->l = NULL;
  if (n->r != NULL) bspDuplicateNode(n->r, &(*p)->r); else (*p)->r = NULL;
}

static void bspDuplicateTree(node_t *n) {
  bspDuplicateNode(n->l, &n->r);
}

static node_t *bspGetNodeForSector(int s) {
  if (!s) return NULL;
  for (unsigned i = 0; i < sc.n; ++i)
    if (sc.p[i].s == s)
      return sc.p[i].n;
  for (unsigned i = 0; i < sc.n; ++i)
    if (!sc.p[i].s) {
      sc.p[i].s = sc.p[i].n->s = s;
      return sc.p[i].n;
    }
  node_t *p = (node_t *)malloc(sizeof(node_t));
  if (p == NULL) return p;
  p->alloc = p->n = p->s = 0;
  p->p = NULL;
  p->l = root;
  bspDuplicateTree(p);
  root = p;
  return bspGetNodeForSector(s);
}

int bspAddLine(int s, int x1, int y1, int x2, int y2, int u, int v, int flags, int t, int du) {
  int a = bspAddVertex(x1, y1);
  int b = bspAddVertex(x2, y2);
  node_t *n = bspGetNodeForSector(s);
  if (n == NULL) return -1;
  if (a < 0 || b < 0) return -1;
  unsigned i;
  if (n->n == n->alloc) {
    if (!n->alloc)
      n->alloc = 4;
    else
      n->alloc *= 2;
    bspline_t *p = (bspline_t *)malloc(n->alloc * sizeof(bspline_t));
    if (p == NULL) return -1;
    for (i = 0; i < n->n; ++i) p[i] = n->p[i];
    if (n->p != NULL) free(n->p);
    n->p = p;
  }
  n->p[n->n].a = a;
  n->p[n->n].b = b;
  n->p[n->n].u1 = u + sqrtf((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1)) + du;
  n->p[n->n].u2 = u;
  n->p[n->n].v = v;
  n->p[n->n].flags = flags;
  n->p[n->n].neigh = NULL;
  n->p[n->n].t = t;
  unsigned j;
  node_t *p;
  for (i = 0; i < sc.n; ++i)
    if (sc.p[i].s != s) {
      p = sc.p[i].n;
      for (j = 0; j < p->n; ++j)
        if (p->p[j].a == b && p->p[j].b == a) {
          n->p[n->n].neigh = p;
          p->p[j].neigh = n;
          break;
        }
      if (n->p[n->n].neigh != NULL) break;
    }
  return n->n++;
}

static int bspIntersect(bspvertex_t p1, bspvertex_t p2, bspvertex_t p3, bspvertex_t p4, bspvertex_t *mp) {
  double dx21 = p2.x - p1.x;
  double dx43 = p4.x - p3.x;
  double dy21 = p2.y - p1.y;
  double dy43 = p4.y - p3.y;
  double dy13 = p1.y - p3.y;
  double t = dy43 * dx21 - dy21 * dx43;
  if (!t) return 0; /* parallel */
  double y, x = (dx21 * (dy13 * dx43 + dy43 * p3.x) - (dy21 * dx43 * p1.x)) / t;
  if (abs(dx21) > abs(dx43)) {
    if (dx21)
      y = p1.y + (x - p1.x) * dy21 / dx21;
    else
      return 0;
  } else if (dx43)
    y = p3.y + (x - p3.x) * dy43 / dx43;
  else
    return 0;
  mp->x = round(x);
  mp->y = round(y);
  return 1;
}

static void bspShowSub(node_t *n) {
  if (n->l != NULL) bspShowSub(n->l);
  if (n->r != NULL) bspShowSub(n->r);
  if (n->p != NULL) {
    grSetColor((n->s * 343) | 3);
    for (unsigned i = 0; i < n->n; ++i)
      edVector(vc.p[n->p[i].a].x, vc.p[n->p[i].a].y, vc.p[n->p[i].b].x, vc.p[n->p[i].b].y);
  }
}

void bspShow() {
  if (root != NULL) bspShowSub(root);
  grSetColor(255);
  for (unsigned i = 0; i < vc.n; ++i) edVertex(vc.p[i].x, vc.p[i].y);
}

struct bsp_count_result {
  unsigned node_count;
  unsigned line_count;
};

static void
bspCountSub(node_t *n, struct bsp_count_result *result)
{
  ++result->node_count;
  result->line_count += n->n;
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
  if (abs(vc.p[p[1]].x - vc.p[p[0]].x) > abs(vc.p[p[1]].y - vc.p[p[0]].y)) {
    for (i = 0; i < n; ++i) {
      k = i;
      for (j = i + 1; j < n; ++j)
        if (vc.p[p[j]].x > vc.p[p[k]].x) k = j;
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
        if (vc.p[p[j]].y > vc.p[p[k]].y) k = j;
      if (k != i) {
        j = p[k];
        p[k] = p[i];
        p[i] = j;
      }
    }
  }
}

static int bspMayConnect(node_t *n, int a, int b) {
  for (unsigned i = 0; i < n->n; ++i) {
    if ((n->p[i].a == a && n->p[i].b == b))
      return 0;
    n->p[i].n = 0;
  }
  return !0;
}

static int bspALine(node_t *n, int a) {
  unsigned j = 0;
  int bo;
  for (unsigned i = 0; i < n->n; ++i)
    if (n->p[i].a == a) {
      bo = 0;
      for (; j < n->n; ++j)
        if (n->p[j].b == a) {
          ++j;
          ++bo;
          break;
        }
      if (!bo) return n->s;
    }
  return 0;
}

static int bspBLine(node_t *n, int b) {
  unsigned j = 0;
  int bo;
  for (unsigned i = 0; i < n->n; ++i)
    if (n->p[i].b == b) {
      bo = 0;
      for (; j < n->n; ++j)
        if (n->p[j].a == b) {
          ++j;
          ++bo;
          break;
        }
      if (!bo) return n->s;
    }
  return 0;
}

static int bspGetPair(node_t *n, unsigned l) {
  node_t *p = n->p[l].neigh;
  unsigned i;
  for (i = 0; i < p->n; ++i)
    if (p->p[i].a == n->p[l].b && p->p[i].b == n->p[l].a) {
      if (p->p[i].neigh != n) printf("pair neighje mashova mutat\n");
      return i;
    }
  return -1;
}

static void bspNoticePair(node_t *n, unsigned l, node_t *p) {
  int i = bspGetPair(n, l);
  if (i != -1) n->p[l].neigh->p[i].neigh = p;
}

static void
bspCleanSub(node_t **n)
{
  if ((*n)->l != NULL) bspCleanSub(&(*n)->l);
  if ((*n)->r != NULL) bspCleanSub(&(*n)->r);
  if ((*n)->l == NULL && (*n)->r == NULL && !(*n)->n) {
    free(*n);
    *n = NULL;
  }
}

void bspCleanTree() {
  if (root != NULL) bspCleanSub(&root);
}

//int bspLoad(FILE *f);

static void
bspBuildSub(node_t *n)
{
  if (n == NULL || n->n < 3) return;
//  if (n->s != 31) return;
//  printf("bspBuildSub(): %d\n", n->n);
  for (unsigned i = 0; i < n->n; ++i) {
    n->p[i].l = n->p[i].r = n->p[i].n = 0;
    const int dx1 = vc.p[n->p[i].b].x - vc.p[n->p[i].a].x;
    const int dy1 = vc.p[n->p[i].b].y - vc.p[n->p[i].a].y;
    for (unsigned j = 0; j < n->n; ++j) {
      const int dx2a = vc.p[n->p[j].a].x - vc.p[n->p[i].a].x;
      const int dy2a = vc.p[n->p[j].a].y - vc.p[n->p[i].a].y;
      const int da = dy1 * dx2a - dy2a * dx1;
      const int dx2b = vc.p[n->p[j].b].x - vc.p[n->p[i].a].x;
      const int dy2b = vc.p[n->p[j].b].y - vc.p[n->p[i].a].y;
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
  for (unsigned i = 1; i < n->n; ++i) {
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
  const int dx1 = vc.p[n->p[j].b].x - vc.p[n->p[j].a].x;
  const int dy1 = vc.p[n->p[j].b].y - vc.p[n->p[j].a].y;
  int l = 0;
  int e = 0;
  int r = 0;
  int f = 0;
  for (unsigned i = 0; i < vc.n; ++i) vc.p[i].s = 0;
  int mina = 0;
  int maxa = 0;
  for (unsigned i = 0; i < n->n; ++i) {
    n->p[i].l = n->p[i].r = 0;
    const int dx2a = vc.p[n->p[i].a].x - vc.p[n->p[j].a].x;
    const int dy2a = vc.p[n->p[i].a].y - vc.p[n->p[j].a].y;
    int da = dy1 * dx2a - dy2a * dx1;
    const int dx2b = vc.p[n->p[i].b].x - vc.p[n->p[j].a].x;
    const int dy2b = vc.p[n->p[i].b].y - vc.p[n->p[j].a].y;
    const int db = dy1 * dx2b - dy2b * dx1;
    if (da < mina) mina = da;
    if (db < mina) mina = db;
    if (da > maxa) maxa = da;
    if (db > maxa) maxa = db;
    if (!da && !vc.p[n->p[i].a].s) {
      ++f;
      ++vc.p[n->p[i].a].s;
    }
    if (!db && !vc.p[n->p[i].b].s) {
      ++f;
      ++vc.p[n->p[i].b].s;
    }
    if (!da && !db) {
      const int dx2 = vc.p[n->p[i].a].x - vc.p[n->p[i].b].x;
      const int dy2 = vc.p[n->p[i].a].y - vc.p[n->p[i].b].y;
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

  n->l = (node_t *)malloc(sizeof(node_t));
  if (n->l == NULL) return;
  n->l->l = n->l->r = NULL;
  n->l->alloc = l + e + f;
  n->l->p = (bspline_t *)malloc(n->l->alloc * sizeof(bspline_t));
  if (n->l->p == NULL) {
    free(n->l);
    n->l = NULL;
    return;
  }
  n->l->n = 0;

  n->r = (node_t *)malloc(sizeof(node_t));
  if (n->r == NULL) {
    free(n->l->p);
    free(n->l);
    n->l = NULL;
    return;
  }
  n->r->l = n->r->r = NULL;
  n->r->alloc = r + e + f;
  n->r->p = (bspline_t *)malloc(n->r->alloc * sizeof(bspline_t));
  if (n->r->p == NULL) {
    free(n->l->p);
    free(n->l);
    n->l = NULL;
    free(n->r);
    n->r = NULL;
    return;
  }
  n->r->n = 0;

  n->r->s = n->l->s = n->s;

  l = r = 0;
  bspline_t tl = { 0 };
  for (unsigned i = 0; i < n->n; ++i) {
    if (n->p[i].r) {
      n->r->p[r++] = n->p[i];
      if (n->p[i].neigh != NULL) bspNoticePair(n, i, n->r);
      continue;
    }
    if (n->p[i].l) {
      n->l->p[l++] = n->p[i];
      if (n->p[i].neigh != NULL) bspNoticePair(n, i, n->l);
      continue;
    }
    const int dx2a = vc.p[n->p[i].a].x - vc.p[n->p[j].a].x;
    const int dy2a = vc.p[n->p[i].a].y - vc.p[n->p[j].a].y;
    const int da = dy1 * dx2a - dy2a * dx1;
    bspvertex_t v;
    v.x = v.y = 0;
    bspIntersect(vc.p[n->p[j].a], vc.p[n->p[j].b], vc.p[n->p[i].a], vc.p[n->p[i].b], &v);
    if ((e = bspGetVertex(v.x, v.y)) == -1) e = bspAddVertex(v.x, v.y);
    vc.p[e].s = 1;
    node_t * const ne = n->p[i].neigh;
    int t = (ne != NULL) ? bspGetPair(n, i) : -1;
    if (t >= 0) {
      /* make sure we have enough space on the neigh node */
      if (ne->p[t].a != e && ne->p[t].b != e && ne->n == ne->alloc) {
        ne->alloc *= 2;
        bspline_t *p = (bspline_t *)malloc(ne->alloc * sizeof(bspline_t));
        for (unsigned j = 0; j < ne->n; ++j) p[j] = ne->p[j];
        free(ne->p);
        ne->p = p;
      }
      tl = ne->p[t];
      ne->p[t] = ne->p[--ne->n];
    }
    t = t != -1;
    if (da > 0) {
      int dx2 = 0;
      int dy2 = 0;
      if (abs(vc.p[n->p[i].b].x - vc.p[n->p[i].a].x) > abs(vc.p[n->p[i].b].y - vc.p[n->p[i].a].y)) {
        dx2 = (n->p[i].u1 - n->p[i].u2) * (vc.p[n->p[i].b].x - v.x);
        dx2 = n->p[i].u2 + (dx2 / (vc.p[n->p[i].b].x - vc.p[n->p[i].a].x));
        if (t) {
          dy2 = (tl.u1 - tl.u2) * (v.x - vc.p[tl.b].x);
          dy2 = tl.u2 + (dy2 / (vc.p[tl.a].x - vc.p[tl.b].x));
        }
      } else {
        dx2 = (n->p[i].u1 - n->p[i].u2) * (vc.p[n->p[i].b].y - v.y);
        dx2 = n->p[i].u2 + (dx2 / (vc.p[n->p[i].b].y - vc.p[n->p[i].a].y));
        if (t) {
          dy2 = (tl.u1 - tl.u2) * (v.y - vc.p[tl.b].y);
          dy2 = tl.u2 + (dy2 / (vc.p[tl.a].y - vc.p[tl.b].y));
        }
      }
      if (n->p[i].a != e) {
        n->l->p[l].a = n->p[i].a;
        n->l->p[l].u1 = n->p[i].u1;
        n->l->p[l].b = e;
        n->l->p[l].u2 = dx2;
        n->l->p[l].v = n->p[i].v;
        n->l->p[l].t = n->p[i].t;
        if (t) {
          n->l->p[l].neigh = ne;
          ne->p[ne->n].a = e;
          ne->p[ne->n].u1 = dy2;
          ne->p[ne->n].b = tl.b;
          ne->p[ne->n].u2 = tl.u2;
          ne->p[ne->n].neigh = n->l;
          ne->p[ne->n].v = tl.v;
          ne->p[ne->n].t = tl.t;
          ++ne->n;
        } else
          n->l->p[l].neigh = NULL;
        ++l;
      }
      if (n->p[i].b != e) {
        n->r->p[r].a = e;
        n->r->p[r].u1 = dx2;
        n->r->p[r].b = n->p[i].b;
        n->r->p[r].u2 = n->p[i].u2;
        n->r->p[r].v = n->p[i].v;
        n->r->p[r].t = n->p[i].t;
        if (t) {
          n->r->p[r].neigh = ne;
          ne->p[ne->n].a = tl.a;
          ne->p[ne->n].u1 = tl.u1;
          ne->p[ne->n].b = e;
          ne->p[ne->n].u2 = dy2;
          ne->p[ne->n].neigh = n->r;
          ne->p[ne->n].v = tl.v;
          ne->p[ne->n].t = tl.t;
          ++ne->n;
        } else
          n->r->p[r].neigh = NULL;
        ++r;
      }
    } else {
      int dx2 = 0;
      int dy2 = 0;
      if (abs(vc.p[n->p[i].b].x - vc.p[n->p[i].a].x) > abs(vc.p[n->p[i].b].y - vc.p[n->p[i].a].y)) {
        dx2 = (n->p[i].u1 - n->p[i].u2) * (vc.p[n->p[i].b].x - v.x);
        dx2 = n->p[i].u2 + (dx2 / (vc.p[n->p[i].b].x - vc.p[n->p[i].a].x));
        if (t) {
          dy2 = (tl.u1 - tl.u2) * (v.x - vc.p[tl.b].x);
          dy2 = tl.u2 + (dy2 / (vc.p[tl.a].x - vc.p[tl.b].x));
        }
      } else {
        dx2 = (n->p[i].u1 - n->p[i].u2) * (vc.p[n->p[i].b].y - v.y);
        dx2 = n->p[i].u2 + (dx2 / (vc.p[n->p[i].b].y - vc.p[n->p[i].a].y));
        if (t) {
          dy2 = (tl.u1 - tl.u2) * (v.y - vc.p[tl.b].y);
          dy2 = tl.u2 + (dy2 / (vc.p[tl.a].y - vc.p[tl.b].y));
        }
      }
      if (n->p[i].a != e) {
        n->r->p[r].a = n->p[i].a;
        n->r->p[r].u1 = n->p[i].u1;
        n->r->p[r].b = e;
        n->r->p[r].u2 = dx2;
        n->r->p[r].v = n->p[i].v;
        n->r->p[r].t = n->p[i].t;
        if (t) {
          n->r->p[r].neigh = ne;
          ne->p[ne->n].a = e;
          ne->p[ne->n].u1 = dy2;
          ne->p[ne->n].b = tl.b;
          ne->p[ne->n].u2 = tl.u2;
          ne->p[ne->n].neigh = n->r;
          ne->p[ne->n].v = tl.v;
          ne->p[ne->n].t = tl.t;
          ++ne->n;
        } else
          n->r->p[r].neigh = NULL;
        ++r;
      }
      if (n->p[i].b != e) {
        n->l->p[l].a = e;
        n->l->p[l].u1 = dx2;
        n->l->p[l].b = n->p[i].b;
        n->l->p[l].u2 = n->p[i].u2;
        n->l->p[l].v = n->p[i].v;
        n->l->p[l].t = n->p[i].t;
        if (t) {
          n->l->p[l].neigh = ne;
          ne->p[ne->n].a = tl.a;
          ne->p[ne->n].u1 = tl.u1;
          ne->p[ne->n].b = e;
          ne->p[ne->n].u2 = dy2;
          ne->p[ne->n].neigh = n->l;
          ne->p[ne->n].v = tl.v;
          ne->p[ne->n].t = tl.t;
          ++ne->n;
        } else
          n->l->p[l].neigh = NULL;
        ++l;
      }
    }
  }

  free(n->p);
  n->p = NULL;

  /* fontos! */
  n->r->n = r;
  n->l->n = l;

  unsigned p[f];
  int t = 0;
  for (unsigned i = 0; i < vc.n; ++i) if (vc.p[i].s) p[t++] = i;
  bspSortVerteces(p, t);
  const int dx2 = vc.p[p[0]].x - vc.p[p[t-1]].x;
  const int dy2 = vc.p[p[0]].y - vc.p[p[t-1]].y;
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
          n->r->p[n->r->n].a = p[i];
          n->r->p[n->r->n].b = p[i-1];
          n->r->p[n->r->n].neigh = NULL;
          n->r->p[n->r->n].flags = LF_NOTHING; /* two sided will be set at save */
          n->r->p[n->r->n].u1 = n->r->p[n->r->n].u2 = n->r->p[n->r->n].v = 0;
          n->r->p[n->r->n].t = 0;
          ++n->r->n;
          ++j;
        }
        if (db && bspMayConnect(n->l, p[i-1], p[i])) {
          n->l->p[n->l->n].a = p[i-1];
          n->l->p[n->l->n].b = p[i];
          n->l->p[n->l->n].flags = LF_NOTHING; /* two sided will be set at save */
          n->l->p[n->l->n].u1 = n->l->p[n->l->n].u2 = n->l->p[n->l->n].v = 0;
          n->l->p[n->l->n].t = 0;
          if (j) {
            n->r->p[n->r->n-1].neigh = n->l;
            n->l->p[n->l->n].neigh = n->r;
          } else
            n->l->p[n->l->n].neigh = NULL;
          ++n->l->n;
        }
      } while (i < t);
    } else {
      do {
        const int da = bspALine(n->l, p[i]);
        const int db = bspBLine(n->r, p[i]);
        ++i;
        j = 0;
        if (da && bspMayConnect(n->l, p[i], p[i-1])) {
          n->l->p[n->l->n].a = p[i];
          n->l->p[n->l->n].b = p[i-1];
          n->l->p[n->l->n].neigh = NULL;
          n->l->p[n->l->n].flags = LF_NOTHING; /* two sided will be set at save */
          n->l->p[n->l->n].u1 = n->l->p[n->l->n].u2 = n->l->p[n->l->n].v = 0;
          n->l->p[n->l->n].t = 0;
          ++n->l->n;
          ++j;
        }
        if (db && bspMayConnect(n->r, p[i-1], p[i])) {
          n->r->p[n->r->n].a = p[i-1];
          n->r->p[n->r->n].b = p[i];
          n->r->p[n->r->n].flags = LF_NOTHING; /* two sided will be set at save */
          n->r->p[n->r->n].u1 = n->r->p[n->r->n].u2 = n->r->p[n->r->n].v = 0;
          n->r->p[n->r->n].t = 0;
          if (j) {
            n->r->p[n->r->n].neigh = n->l;
            n->l->p[n->l->n-1].neigh = n->r;
          } else
            n->r->p[n->r->n].neigh = NULL;
          ++n->r->n;
        }
      } while (i < t);
    }
  }

  e = rand() | 3;
  for (unsigned i = 0; i < n->r->n; ++i) n->r->p[i].c = e;
  e = rand() | 3;
  for (unsigned i = 0; i < n->l->n; ++i) n->l->p[i].c = e;

  grBegin();
  bspShowSub(n);
  grEnd();
//  SDL_Delay(100);
  bspBuildSub(n->l);
  bspBuildSub(n->r);
  n->n = 0;
}

static void
bspBuildSearch(node_t *n)
{
  if (!n->n) {
    if (n->l != NULL) bspBuildSearch(n->l);
    if (n->r != NULL) bspBuildSearch(n->r);
  } else {
    int e = rand() | 3;
    for (unsigned i = 0; i < n->n; ++i) n->p[i].c = e;
    bspBuildSub(n);
  }
}

void bspBuildTree() {
  bspCleanTree();
  printf("bspBuildTree():\n vertex: %d\n", vc.n);
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
  vc.alloc = 8;
  vc.p = (bspvertex_t *)malloc(vc.alloc * sizeof(bspvertex_t));
  if (vc.p == NULL) return 0;
  vc.n = 0;

  sc.alloc = 8;
  sc.p = (bspsector_t *)malloc(sc.alloc * sizeof(bspsector_t));
  if (sc.p == NULL) goto end1;
  sc.n = 1;

  root = (node_t *)malloc(sizeof(node_t));
  if (root == NULL) goto end2;
  root->l = root->r = NULL;
  root->alloc = 0;
  root->p = NULL;
  root->n = 0;

  sc.p[0].n = root;
  sc.p[0].s = 0;

  return !0;
 end2:
  free(sc.p);
 end1:
  free(vc.p);
  return 0;
}

static void
bspDelNode(node_t *n)
{
  if (n->l != NULL) bspDelNode(n->l);
  if (n->r != NULL) bspDelNode(n->r);
  if (n->p != NULL) free(n->p);
  free(n);
}

void bspDone() {
  if (root != NULL) bspDelNode(root);
  free(sc.p);
  sc.n = sc.alloc = 0;
  free(vc.p);
  vc.n = vc.alloc = 0;
}

static void
bspPrintTreeSub(node_t *n, unsigned d)
{
  for (unsigned i = 0; i < d; ++i) putchar(' ');
  printf("%d\n", n->n);
  ++d;
  if (n->l != NULL) bspPrintTreeSub(n->l, d);
  if (n->r != NULL) bspPrintTreeSub(n->r, d);
}

void bspPrintTree() {
  if (root != NULL) bspPrintTreeSub(root, 0);
}

typedef struct {
  short x : 16;
  short y : 16;
} __attribute__((packed)) fbspvertex_t;

typedef struct {
  short a : 16;
  short b : 16;
  unsigned char flags : 8;
  unsigned short u1 : 16;
  unsigned short u2 : 16;
  unsigned short v : 16;
  unsigned int t : 32;
} __attribute__((packed)) fbspline_t;

int bspSave(FILE *f) {
  unsigned i, j, k;
  fbspvertex_t fv;
  fbspline_t fl;

  void bspSaveSub(node_t *n) {
    if (n == NULL) {
      i = 0;
      fwrite(&i, sizeof(int), 1, f);
      return;
    }
    i = n->n + 1;
    fwrite(&i, sizeof(int), 1, f);
    fwrite(&n->s, sizeof(int), 1, f);
    if (n->n) {
      j = n->p[0].a;
      for (i = 0; i < n->n; ++i) n->p[i].n = 0;
      k = 0;
      for (i = 0; i < n->n; ++i) {
        if (n->p[i].a == j && !n->p[i].n) {
          fl.a = n->p[i].a;
          fl.b = n->p[i].b;
          fl.flags = LF_NOTHING;
          if (n->p[i].neigh != NULL) fl.flags |= LF_TWOSIDED;
          fl.u1 = n->p[i].u1;
          fl.u2 = n->p[i].u2;
          fl.v = n->p[i].v;
          fl.t = n->p[i].t;
          fwrite(&fl, sizeof(fbspline_t), 1, f);
          j = n->p[i].b;
          ++n->p[i].n;
          i = 0;
          ++k;
        }
      }
      if (k != n->n) {
        printf("open sector!!! (%d) k=%d n->n=%d\n", n->s, k, n->n);
        for (i = 0; i < n->n; ++i)
          printf(" %d %d\n", n->p[i].a, n->p[i].b);
        // halvany lila workaround ...
        for (; k < n->n; ++k)
          fwrite(&fl, sizeof(fbspline_t), 1, f);
      }
    }
    bspSaveSub(n->l);
    bspSaveSub(n->r);
  }

  fwrite(&vc.n, sizeof(int), 1, f);
  for (i = 0; i < vc.n; ++i) {
    fv.x = vc.p[i].x;
    fv.y = vc.p[i].y;
    fwrite(&fv, sizeof(fbspvertex_t), 1, f);
  }
  struct bsp_count_result counts;
  bspCount(&counts);
//  bspPrintTree();
  fwrite(&counts.node_count, sizeof(unsigned), 1, f);
  fwrite(&counts.line_count, sizeof(unsigned), 1, f);
  bspSaveSub(root);
  return !0;
}
