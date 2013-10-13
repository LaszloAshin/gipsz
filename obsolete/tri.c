#include <SDL/SDL_opengl.h>
#include "tri.h"
#include "mm.h"
#include "tex.h"
#include "console.h"

static struct {
  tri_t *p;
  int alloc, n;
} tc = { NULL, 0, 0 };

typedef struct {
  unsigned d;
  tri_t *p;
} idx_t;
static idx_t *idx = NULL, *idxs = NULL;
static int idxsize = 0;

int triAdd(tri_t *tri) {
  if (tc.n == tc.alloc) {
    int na = tc.alloc * 2;
    tri_t *p = mmAlloc(na * sizeof(tri_t));
    if (p == NULL) return 0;
    tc.alloc = na;
    int i;
    for (i = 0; i < tc.n; ++i) p[i] = tc.p[i];
    mmFree(tc.p);
    tc.p = p;
  }
  tc.p[tc.n] = *tri;
  ++tc.n;
  return !0;
}

int triBegin() {
  if (tc.p == NULL) {
    tc.alloc = 8;
    tc.p = mmAlloc(tc.alloc * sizeof(tri_t));
  }
  tc.n = 0;
  return !0;
}

static void triSort() {
  int i, bckt[256], k, l;
  unsigned char *cp;

  void triSortSub(idx_t *dest, idx_t *src, int offs) {
    for (i = 0; i < 256; ++i) bckt[i] = 0;
    cp = (unsigned char *)&src->d + offs;
    for (i = 0; i < tc.n; ++i, cp += sizeof(idx_t))
      ++bckt[*cp];
    k = 0;
    for (i = 0; i < 256; ++i) {
      l = bckt[i];
      bckt[i] = k;
      k += l;
    }
    cp = (unsigned char *)&src->d + offs;
    for (i = 0; i < tc.n; ++i, cp += sizeof(idx_t))
      dest[bckt[*cp]++] = src[i];
  }

  if (idxsize < tc.n) {
    if (idxs != NULL) {
      mmFree(idxs);
      idxs = NULL;
    }
    if (idx != NULL) mmFree(idx);
    idxsize = 0;
    idx = mmAlloc(tc.n * sizeof(idx_t));
    if (idx == NULL) return;
    idxs = mmAlloc(tc.n * sizeof(idx_t));
    if (idxs == NULL) {
      mmFree(idx);
      idx = NULL;
      return;
    }
    idxsize = tc.n;
  }
  tri_t *t = tc.p;
  double x, y, z;
  for (i = 0; i < tc.n; ++i, ++t) {
    idx[i].p = t;
    x = (t->v[0].x + t->v[1].x + t->v[2].x) * 0.3333;
    y = (t->v[0].y + t->v[1].y + t->v[2].y) * 0.3333;
    z = (t->v[0].z + t->v[1].z + t->v[2].z) * 0.3333;
    idx[i].d = x * x + y * y + z * z;
  }
  return;
  int j;
  idx_t *tmp;
  for (j = 0; j < sizeof(int); ++j) {
    triSortSub(idxs, idx, j);
    tmp = idx;
    idx = idxs;
    idxs = tmp;
  }
}

int triEnd() {
  if (!tc.n) return !0;
  triSort();
  int i, j, ct;
  tri_t *t;
  idx_t *ix = idx;// + tc.n - 1;
  ct = idx->p->tex;
  texSelectTexture(ct);
  glBegin(GL_TRIANGLES);
  for (i = tc.n; i; --i, ++ix) {
    t = ix->p;
    if (ct != t->tex) {
      ct = t->tex;
      glEnd();
      texSelectTexture(ct);
      glBegin(GL_TRIANGLES);
    }
    glNormal3dv((double *)&t->n);
    for (j = 0; j < 3; ++j) {
      glColor3dv((double *)(t->c + j));
      glTexCoord2dv((double *)(t->tc + j));
      glVertex3dv((double *)(t->v + j));
    }
  }
  glEnd();
  return !0;
}

void triFree() {
  cmsg(MLDBG, "triFree");
  if (idxs != NULL) {
    mmFree(idxs);
    idxs = NULL;
  }
  if (idx != NULL) {
    mmFree(idx);
    idx = NULL;
  }
  idxsize = 0;
  if (tc.p != NULL) {
    mmFree(tc.p);
    tc.p = NULL;
  }
  tc.alloc = tc.n = 0;
}

int triHowMany(void) { return tc.n; }
