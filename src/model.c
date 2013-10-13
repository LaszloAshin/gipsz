#include <string.h>
#include <math.h>
#include <SDL/SDL_opengl.h>
#include "model.h"
#include "mm.h"
#include "tex.h"
#include "obj.h"
#include "console.h"

typedef struct {
  float x, y, z;
  float s, t;
/*  float sx, sy, sz;
  float tx, ty, tz;*/
  float nx, ny, nz;
} vert_t;

typedef struct {
  int a, b, c;
} tri_t;

typedef struct {
  float beta, gamma;
  float alpha;
} stat_t;

typedef struct {
  int name;
  int nverts;
  int ntris;
  vert_t *vp;
  tri_t *tp;
  int tex;
  GLuint list;
} model_t;

static struct {
  model_t *p;
  int alloc, n;
} mc;

#include "model_predefs.c"

void *modelGetForName(int name) {
  if (!name) return NULL;
  int i;
  for (i = 0; i < mc.n; ++i)
    if (mc.p[i].name == name) return mc.p + i;
  return NULL;
}

void modelComputeNormals(model_t *m) {
  int *map;
  int i, j;
  struct n_s { float x, y, z; } *n;
  float ax, ay, az, bx, by, bz, q;

  if (m == NULL || !m->ntris || !m->nverts) return;
  n = (struct n_s *)mmAlloc(m->ntris * sizeof(struct n_s));
  if (n == NULL) return;

  /* compute normals of polygons */
  for (i = 0; i < m->ntris; ++i) {
    ax = m->vp[m->tp[i].b].x - m->vp[m->tp[i].a].x;
    ay = m->vp[m->tp[i].b].y - m->vp[m->tp[i].a].y;
    az = m->vp[m->tp[i].b].z - m->vp[m->tp[i].a].z;
    bx = m->vp[m->tp[i].c].x - m->vp[m->tp[i].a].x;
    by = m->vp[m->tp[i].c].y - m->vp[m->tp[i].a].y;
    bz = m->vp[m->tp[i].c].z - m->vp[m->tp[i].a].z;
    n[i].x = ay * bz - az * by;
    n[i].y = az * bx - ax * bz;
    n[i].z = ax * by - ay * bx;
  }

  /* create map which associates each vertex to another with the same position */
  map = (int *)mmAlloc(m->nverts * sizeof(int));
  if (map == NULL) {
    mmFree(n);
    return;
  }

  for (i = 0; i < m->nverts; ++i) {
    for (j = 0; j < i; ++j)
      if (fabsf(m->vp[i].x - m->vp[j].x) < 0.01 &&
          fabsf(m->vp[i].y - m->vp[j].y) < 0.01 &&
          fabsf(m->vp[i].z - m->vp[j].z) < 0.01) {
        map[i] = j;
        break;
      }
    if (j == i) map[i] = j;
  }

  /* reset normals */
  for (i = 0; i < m->nverts; ++i) {
    m->vp[i].nx = m->vp[i].ny = m->vp[i].nz = 0.0;
  }

  /* compute sum of normal coords in all mapped verteces */
  for (i = 0; i < m->ntris; ++i) {
    m->vp[map[m->tp[i].a]].nx += n[i].x;
    m->vp[map[m->tp[i].a]].ny += n[i].y;
    m->vp[map[m->tp[i].a]].nz += n[i].z;

    m->vp[map[m->tp[i].b]].nx += n[i].x;
    m->vp[map[m->tp[i].b]].ny += n[i].y;
    m->vp[map[m->tp[i].b]].nz += n[i].z;

    m->vp[map[m->tp[i].c]].nx += n[i].x;
    m->vp[map[m->tp[i].c]].ny += n[i].y;
    m->vp[map[m->tp[i].c]].nz += n[i].z;
  }

  /* normalize normal vectors */  
  for (i = 0; i < m->nverts; ++i) {
    if (map[i] == i) {
      q = 1 / sqrtf(m->vp[i].nx * m->vp[i].nx + m->vp[i].ny * m->vp[i].ny + m->vp[i].nz * m->vp[i].nz);
      m->vp[i].nx *= q;
      m->vp[i].ny *= q;
      m->vp[i].nz *= q;
    } else {
      m->vp[i].nx = m->vp[map[i]].nx;
      m->vp[i].ny = m->vp[map[i]].ny;
      m->vp[i].nz = m->vp[map[i]].nz;
    }
  }

  mmFree(map);
  mmFree(n);
}

void *modelAdd(int name) {
  if (!name) return NULL;
  model_t *p = modelGetForName(name);
  if (p != NULL) return NULL;
  if (mc.n == mc.alloc) {
    int na = mc.alloc * 2;
    p = (model_t *)mmAlloc(na * sizeof(model_t));
    if (p == NULL) return p;
    int i;
    for (i = 0; i < mc.n; ++i) p[i] = mc.p[i];
    mmFree(mc.p);
    mc.p = p;
    mc.alloc = na;
  }
  p = mc.p + mc.n++;
  p->name = name;
  p->nverts = p->ntris = 0;
  p->vp = NULL;
  p->tp = NULL;
  p->list = 0;
  switch (name) {
    case 1:
      modelCreateTube(p, 8.0, 64.0, 16);
      break;
    case 2:
      modelCreateElbow(p, 8.0, 16.0, 16);
      break;
    case 3:
      modelCreateSphere(p, 256.0, 40);
      break;
    case 4:
      modelCreateCube(p, 16.0);
      break;
    case 5:
      modelCreateTorus(p, 48.0, 40);
      break;
  }
  modelComputeNormals(p);
  p->list = glGenLists(1);
  if (p->list) {
    glNewList(p->list, GL_COMPILE);
    glVertexPointer(3, GL_FLOAT, sizeof(vert_t), &p->vp->x);
    glEnableClientState(GL_VERTEX_ARRAY);
    glNormalPointer(GL_FLOAT, sizeof(vert_t), &p->vp->nx);
    glEnableClientState(GL_NORMAL_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(vert_t), &p->vp->s);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    if (!p->ntris)
      glDrawArrays(GL_POINTS, 0, p->nverts);
    else
      glDrawElements(GL_TRIANGLES, p->ntris * 3, GL_UNSIGNED_INT, p->tp);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glEndList();
  }
  return p;
}

void *modelInitStat(void *mp) {
  int s = sizeof(stat_t);
  stat_t *p = mmAlloc(s);
  if (p == NULL) return p;
  memset(p, 0, s);
  return p;
}

void modelSetStatAngle(void *sp, float a, float b, float c) {
  stat_t *s = sp;

  s->alpha = a;
  s->beta = b;
  s->gamma = c;
}

void modelUpdate(void *mp, void *sp) {
/*  model_t *m = mp;
  stat_t *s = sp;

  switch (m->name) {
    case 2:
//      cmsg(MLINFO, "&a=%08x &b=%08x &c=%08x", &s->alpha, &s->beta, &s->gamma);
      break;
  }*/
}

void modelDraw(void *mp, void *sp) {
  model_t *m = mp;
  stat_t *s = sp;
  float ma[16];
  float sina, sinb, sing;
  float cosa, cosb, cosg;

  sina = sinf(s->alpha);
  sinb = sinf(s->beta);
  sing = sinf(s->gamma);
  cosa = cosf(s->alpha);
  cosb = cosf(s->beta);
  cosg = cosf(s->gamma);
  ma[0] =  cosb * cosg;
  ma[1] =  cosa * sing + sina * sinb * cosg;
  ma[2] =  sina * sing - cosa * sinb * cosg;
  ma[4] = -cosb * sing;
  ma[5] =  cosa * cosg - sina * sinb * sing;
  ma[6] =  sina * cosg + cosa * sinb * sing;
  ma[8] =  sinb;
  ma[9] = -sina * cosb;
  ma[10] = cosa * cosb;
  ma[3] = ma[7] = ma[11] = ma[12] = ma[13] = ma[14] = 0.0;
  ma[15] =  1.0;
  glMultMatrixf(ma);
  if (m->tex) {
    glEnable(GL_TEXTURE_2D);
    texLoadTexture(m->tex, 0);
    texSelectTexture(m->tex);
  }
  glColor3f(1.0, 1.0, 1.0);
  if (m->list) {
    glCallList(m->list);
  } else if (m->ntris && m->nverts) {
    glVertexPointer(3, GL_FLOAT, sizeof(vert_t), &m->vp->x);
    glEnableClientState(GL_VERTEX_ARRAY);
    glNormalPointer(GL_FLOAT, sizeof(vert_t), &m->vp->nx);
    glEnableClientState(GL_NORMAL_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(vert_t), &m->vp->s);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    if (!m->ntris)
      glDrawArrays(GL_POINTS, 0, m->nverts);
    else
      glDrawElements(GL_TRIANGLES, m->ntris * 3, GL_UNSIGNED_INT, m->tp);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
  }
  glDisable(GL_TEXTURE_2D);
}

int modelInit() {
  mc.alloc = 8;
  mc.p = (model_t *)mmAlloc(mc.alloc * sizeof(model_t));
  if (mc.p == NULL) return 0;
  mc.n = 0;
  return !0;
}

void modelFlush() {
  objFlush();
  int i;
  for (i = 0; i < mc.n; ++i) {
    if (mc.p[i].vp != NULL) {
      mmFree(mc.p[i].vp);
      mc.p[i].vp = NULL;
    }
    if (mc.p[i].tp != NULL) {
      mmFree(mc.p[i].tp);
      mc.p[i].tp = NULL;
    }
    if (mc.p[i].list) {
      glDeleteLists(mc.p[i].list, 1);
      mc.p[i].list = 0;
    }
  }
  mc.n = 0;
}

void modelDone() {
  modelFlush();
  if (mc.p != NULL) {
    mmFree(mc.p);
    mc.p = NULL;
  }
  mc.alloc = 0;
}
