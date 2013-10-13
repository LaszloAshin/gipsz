#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "bsp2.h"
#include "ed.h"
#include "gr.h"
#include <SDL/SDL.h>

typedef struct {
  int x, y;
} vertex_t;

typedef struct {
  int a, b;
} line_t;

typedef struct node_s {
  line_t *p;
  unsigned s, alloc, n;
  struct node_s *l, *r;
} node_t;

static struct {
  vertex_t *p;
  unsigned alloc, n;
} vc;

static node_t *root = NULL;

static int bspGetVertex(int x, int y) {
  int i;
  for (i = 0; i < vc.n; ++i)
    if (vc.p[i].x == x && vc.p[i].y == y)
      return i;
  return -1;
}

static int bspAddVertex(int x, int y) {
  int i;
  if ((i = bspGetVertex(x, y)) >= 0) return i;
  if (vc.n == vc.alloc) {
    vc.alloc *= 2;
    vertex_t *p = (vertex_t *)malloc(vc.alloc * sizeof(vertex_t));
    if (p == NULL) return -1;
    for (i = 0; i < vc.n; ++i) p[i] = vc.p[i];
    free(vc.p);
    vc.p = p;
  }
  vc.p[vc.n].x = x;
  vc.p[vc.n].y = y;
  return vc.n++;
}

static void bspShowSub(node_t *n) {
  static int i;
  if (n->l != NULL) bspShowSub(n->l);
  if (n->r != NULL) bspShowSub(n->r);
  if (n->p != NULL) {
    grSetColor(n->s | 3);
    for (i = 0; i < n->n; ++i)
      edVector(vc.p[n->p[i].a].x, vc.p[n->p[i].a].y, vc.p[n->p[i].b].x, vc.p[n->p[i].b].y);
  }
}

void bspShow() {
  int i;

  if (root != NULL) bspShowSub(root);
  grSetColor(255);
  for (i = 0; i < vc.n; ++i) edVertex(vc.p[i].x, vc.p[i].y);
}

int bspCountNodes() {
  int bspCountNodesSub(node_t *n) {
    if (n == NULL) return 0;
    return 1 + bspCountNodesSub(n->l) + bspCountNodesSub(n->r);
  }

  return bspCountNodesSub(root);
}

static void bspDuplicateTree(node_t *n) {
  void bspDuplicateNode(node_t *n, node_t **p) {
    *p = (node_t *)malloc(sizeof(node_t));
    if (*p == NULL) return;
    p->s = p->alloc = p->n = 0;
    if (n->l != NULL) bspDuplicateNode(n->l, &(*p)->l); else (*p)->l = NULL;
    if (n->r != NULL) bspDuplicateNode(n->r, &(*p)->r); else (*p)->r = NULL;
  }

  bspDuplicateNode(n->l, &n->r);
}

static node_t *bspGetNodeForSector(unsigned s) {
  node_t *q = NULL, *p = root;
  while (p != NULL && p->s != s)
    if (s > p->s) {
      q = p;
      p = p->l;
    } else {
      q = p;
      p = p->r;
    }
  if (p == NULL) {
    p = (node_t *)malloc(sizeof(node_t));
    if (p == NULL) return NULL;
    p->l = p->r = NULL;
    p->alloc = 8;
    p->p = (line_t *)malloc(p->alloc * sizeof(line_t));
    p->n = 0;
    if (q != NULL) {
      if (s > q->s)
        q->l = p;
      else
        q->r = p;
    } else
      root = p;
  }
  return p;
}

void bspAddLine(unsigned s, int x1, int y1, int x2, int y2) {
  int a = bspAddVertex(x1, y1);
  int b = bspAddVertex(x2, y2);
  node_t *n = bspGetNodeForSector(s);
  if (n == NULL) return;
  if (n->n == n->alloc) {
    n->alloc *= 2;
    line_t *p = (line_t *)malloc(n->alloc * sizeof(line_t));
    unsigned i;
    for (i = 0; i < n->n; ++i) p[i] = n->p[i];
    free(n->p);
    n->p = p;
  }
  n->p[n->n].a = a;
  n->p[n->n].b = b;
  ++n->n;
}

void bspBuildTree() {
}

int bspInit() {
  vc.alloc = 8;
  vc.p = (vertex_t *)malloc(vc.alloc * sizeof(vertex_t));
  if (vc.p == NULL) return 0;
  vc.n = 0;

  root = NULL;
  return !0;
}

void bspDone() {
  void bspDelNode(node_t *n) {
    if (n->l != NULL) bspDelNode(n->l);
    if (n->r != NULL) bspDelNode(n->r);
    if (n->p != NULL) free(n->p);
    free(n);
  }
  if (root != NULL) bspDelNode(root);
  free(vc.p);
  vc.n = vc.alloc = 0;
}
