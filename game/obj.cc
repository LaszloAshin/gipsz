#include <SDL/SDL_opengl.h>
#include "obj.h"
#include "mm.h"
#include "model.h"
#include "bsp.h"
#include "player.h"
#include "console.h"

#include <math.h>

typedef struct Obj {
  int name;
  float x, y, z;
  Model *model;
  Stat *stat;
} obj_t;

static struct {
  obj_t *p;
  int alloc, n;
} oc;

Obj *objGetForName(int name) {
  if (!name) return NULL;
  int i;
  for (i = 0; i < oc.n; ++i)
    if (oc.p[i].name == name) return oc.p + i;
  return NULL;
}

Obj *objAdd(int name, int modelname, float x, float y, float z) {
  Model *model;

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

struct fobject {
  int type;
  int x, y, z;
  float ra, rb, rc;
};

int
loadObject(struct fobject *fo, FILE *fp)
{
  char buf[2 + 3 * 2 + 2], *p = buf;
  const size_t rd = fread(buf, 1, sizeof(buf), fp);
  if (rd != sizeof(buf)) return -1;
  fo->type = *(short *)p;
  fo->x = *(short *)(p + 2);
  fo->y = *(short *)(p + 4);
  fo->z = *(short *)(p + 6);
  const unsigned rot = *(short *)(p + 8);
  fo->ra = (rot & 7) * M_PI / 4.0f;
  fo->rb = ((rot >> 3) & 7) * M_PI / 4.0f;
  fo->rc = ((rot >> 6) & 7) * M_PI / 4.0f;
  return 0;
}

int objLoad(FILE *fp) {
  unsigned n;

  objFlush();
  if (fread(&n, sizeof(unsigned), 1, fp) != 1) return 0;
  for (unsigned i = 0; i < n; ++i) {
    struct fobject fo;
    if (loadObject(&fo, fp)) return 0;
    switch (fo.type) {
      case 0:
        plSetPosition(fo.x, fo.y, fo.z, fo.rc + M_PI / 2.0f);
        break;
      default: {
        obj_t *o = objAdd(i + 1, fo.type, fo.x, fo.y, fo.z);
        if (o == NULL) return 0;
        modelSetStatAngle(o->stat, fo.ra, fo.rb, fo.rc);
        break;
      }
    }
  }
  return !0;
}
