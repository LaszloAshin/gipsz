#include <SDL/SDL_opengl.h>
#include "obj.h"
#include "mm.h"
#include "model.h"
#include "bsp.h"
#include "player.h"
#include "console.h"

typedef struct {
  int name;
  float x, y, z;
  void *model;
  void *stat;
  node_t *node;
} obj_t;

static struct {
  obj_t *p;
  int alloc, n;
} oc;

void *objGetForName(int name) {
  if (!name) return NULL;
  int i;
  for (i = 0; i < oc.n; ++i)
    if (oc.p[i].name == name) return oc.p + i;
  return NULL;
}

void *objAdd(int name, int modelname, float x, float y, float z) {
  void *model;

  if (!name) return NULL;
  obj_t *p = objGetForName(name);
  if (p != NULL) return NULL;
  if (oc.n == oc.alloc) {
    int na = oc.alloc * 2;
    p = (obj_t *)mmAlloc(na * sizeof(obj_t));
    if (p == NULL) return p;
    int i;
    for (i = 0; i < oc.n; ++i) p[i] = oc.p[i];
    mmFree(oc.p);
    oc.p = p;
    oc.alloc = na;
  }
  p = oc.p + oc.n++;
  p->name = name;
  p->x = x;
  p->y = y;
  p->z = z;
  model = modelGetForName(modelname);
  if (model == NULL) model = modelAdd(modelname);
  p->model = model;
  p->stat = modelInitStat(model);
  return p;
}

void objUpdate() {
  int i;
  obj_t *o;

  for (i = oc.n, o = oc.p; i; --i, ++o) {
    o->node = bspGetNodeForCoords(o->x, o->y, o->z);
    modelUpdate(o->model, o->stat);
  }
}

void objDrawObjects() {
  int i;
  obj_t *o;

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  /* default matrixmode in objDrawObjects is modelview! */
  glMatrixMode(GL_MODELVIEW);
  for (i = oc.n, o = oc.p; i; --i, ++o) {
    if (o->node == NULL || !(o->node->flags & NF_VISIBLE)) continue;
    glLoadIdentity();
    glTranslatef(o->x, o->y, o->z);
    modelDraw(o->model, o->stat);
  }
  glLoadIdentity();
}

int objInit() {
  oc.alloc = 8;
  oc.p = (obj_t *)mmAlloc(oc.alloc * sizeof(obj_t));
  if (oc.p == NULL) return 0;
  oc.n = 0;
  return !0;
}

void objFlush() {
  int i;

  for (i = 0; i < oc.n; ++i)
    if (oc.p[i].stat != NULL)
      mmFree(oc.p[i].stat);
  oc.n = 0;
}

void objDone() {
  objFlush();
  if (oc.p != NULL) {
    mmFree(oc.p);
    oc.p = NULL;
  }
  oc.alloc = 0;
}

typedef struct {
  short type : 16;
  short x, y, z : 16;
  short rot : 16;
} __attribute__((packed)) fobject_t;

int objLoad(FILE *fp) {
  fobject_t fo;
  int i, n;
  obj_t *o;
  float alpha, beta, gamma;

  objFlush();
  if (fread(&n, sizeof(int), 1, fp) != 1) return 0;
  for (i = 0; i < n; ++i) {
    if (fread(&fo, sizeof(fobject_t), 1, fp) != 1) return 0;
    alpha =  (fo.rot & 7) * 0.7854;
    beta  = ((fo.rot >> 3) & 7) * 0.7854;
    gamma = ((fo.rot >> 6) & 7) * 0.7854;
    switch (fo.type) {
      case 0:
        plSetPosition(fo.x, fo.y, fo.z, gamma + 1.5708);
        break;
      default:
        o = objAdd(i + 1, fo.type, fo.x, fo.y, fo.z);
        if (o == NULL) return 0;
        modelSetStatAngle(o->stat, alpha, beta, gamma);
        break;
    }
  }
  return !0;
}
