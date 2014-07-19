#include <stdlib.h>
#include "vertex.h"
#include "line.h"
#include "ed.h"
#include "gr.h"

vc_t vc;
vertex_t *sv = NULL;

vertex_t *edAddVertex(int x, int y) {
  if (vc.n == vc.alloc) {
    unsigned na = vc.alloc * 2;
    vertex_t *p = (vertex_t *)malloc(na * sizeof(vertex_t));
    if (p == NULL) return p;
    for (unsigned i = 0; i < vc.n; ++i) p[i] = vc.p[i];
    sv += p - vc.p;
    free(vc.p);
    vc.p = p;
    vc.alloc = na;
  }
  vc.p[vc.n].x = x;
  vc.p[vc.n].y = y;
  ++vc.n;
  return vc.p + vc.n - 1;
}

vertex_t *edGetVertex(int x, int y) {
  for (unsigned i = 0; i < vc.n; ++i)
    if (vc.p[i].x == x && vc.p[i].y == y)
      return vc.p + i;
  return NULL;
}

void edDelVertex(vertex_t *p) {
  const unsigned i = p - vc.p;
  if (p == NULL || i > vc.n) return;
  for (unsigned j = 0; j < lc.n; ++j)
    if (lc.p[j].a == (int)i || lc.p[j].b == (int)i) {
      lc.p[j] = lc.p[--lc.n];
      sl = NULL;
      --j;
    }
  --vc.n;
  for (unsigned j = 0; j < lc.n; ++j) {
    if (lc.p[j].a == (int)vc.n) lc.p[j].a = i;
    if (lc.p[j].b == (int)vc.n) lc.p[j].b = i;
  }
  vc.p[i] = vc.p[vc.n];
  sv = NULL;
}

void edMouseButtonVertex(int mx, int my, int button) {
  vertex_t *v = edGetVertex(mx, my);
  if (snap && v == NULL && sv != NULL && sv->md <= gs*gs) v = sv;
  if (button == 1) {
    if (v != NULL) {
      sv = v;
      moving = 1;
    } else {
      edAddVertex(mx, my);
      grBegin();
      grSetColor(255);
      edVertex(mx, my);
      grEnd();
    }
  } else {
    if (moving) {
      moving = 0;
      edScreen();
    }
    if (button == 3 && v != NULL) {
      edDelVertex(v);
      edScreen();
    }
  }
}

void edMouseMotionVertex(int mx, int my, int umx, int umy) {
  (void)umx;
  (void)umy;
  grBegin();
  grSetPixelMode(PMD_XOR);
  grSetColor(255);
  if (omx != -1 || omy != -1) edOctagon(omx, omy, 4);
  edOctagon(mx, my, 4);
  grSetPixelMode(PMD_SET);
  grEnd();
  if (moving && sv != NULL) {
    if (edGetVertex(mx, my) == NULL) {
      grBegin();
      grSetColor(254);
      grSetPixelMode(PMD_XOR);
      edVertex(sv->x, sv->y);
      edVertex(mx, my);
      grSetPixelMode(PMD_SET);
      grEnd();
      sv->x = mx;
      sv->y = my;
    }
  }
}

void edKeyboardVertex(int key) {
  (void)key;
}

int pszt(vertex_t p1, vertex_t p2, int mx, int my) {
  int dx21, dy21;
  if (p1.x > p2.x) {
    vertex_t p = p1;
    p1 = p2;
    p2 = p;
  }
  dx21 = p2.x - p1.x;
  dy21 = p2.y - p1.y;
  int x = my - p1.y;
  x *= dx21 * dy21;
  x += p1.x * dy21 * dy21 + mx * dx21 * dx21;
  x /= dy21 * dy21 + dx21 * dx21;
  int y;
  if (x < p1.x) {
    x = p1.x;
    y = p1.y;
  } else if (x > p2.x) {
    x = p2.x;
    y = p2.y;
  } else {
    if (abs(dx21) > abs(dy21)) {
      if (dx21)
        y = p1.y + (x - p1.x) * dy21 / dx21;
      else
        return 0;
    } else if (dy21)
      y = my - (x - mx) * dx21 / dy21;
    else
      return 0;
    if (p1.y > p2.y) {
      vertex_t p = p1;
      p1 = p2;
      p2 = p;
    }
    if (y < p1.y) {
      y = p1.y;
      x = p1.x;
    } else if (y > p2.y) {
      y = p2.y;
      x = p2.x;
    }
  }
  dx21 = mx - x;
  dy21 = my - y;
  return dx21 * dx21 + dy21 * dy21;
}
