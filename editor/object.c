#include <stdlib.h>
#include <math.h>
#include "object.h"
#include "gr.h"
#include "ed.h"

oc_t oc;

object_t *so = NULL;

static const objprop_t objprop[] = {
  { OT_START, "start", 16, 255 },
  { OT_TUBE,  "tube",  8, 250 },
  { OT_ELBOW, "elbow", 8, 250 },
  { OT_START, NULL,     0,   0 }
};

char *edGetObjectProperties(objtype_t wh, int *r, int *c) {
  int i;

  for (i = 0; objprop[i].name != NULL; ++i)
    if (objprop[i].typ == wh) {
      if (r != NULL) *r = objprop[i].r;
      if (c != NULL) *c = objprop[i].c;
      return objprop[i].name;
    }
  return NULL;
}

object_t *edGetObject(int x, int y) {
  int i;
  for (i = 0; i < oc.n; ++i)
    if (oc.p[i].x == x && oc.p[i].y == y)
      return oc.p + i;
  return NULL;
}

int edAddObject(objtype_t wh, int x, int y, int z, int a, int b, int c) {
  int i, na;

  if (oc.n == oc.alloc) {
    na = oc.alloc * 2;
    object_t *p = (object_t *)malloc(na * sizeof(object_t));
    if (p == NULL) return -1;
    for (i = 0; i < oc.n; ++i) p[i] = oc.p[i];
    free(oc.p);
    oc.p = p;
    oc.alloc = na;
  }
  oc.p[oc.n].x = x;
  oc.p[oc.n].y = y;
  oc.p[oc.n].z = z;
  oc.p[oc.n].a = a;
  oc.p[oc.n].b = b;
  oc.p[oc.n].c = c;
  oc.p[oc.n].what = wh;
  return oc.n++;
}

void edDelObject(object_t *o) {
  int i = o - oc.p;
  if (i < 0) return;
  oc.p[i] = oc.p[--oc.n];
}

void edMouseButtonObject(int mx, int my, int button) {
  object_t *o = edGetObject(mx, my);
  if (button == 1) {
    if (o != NULL) {
      moving = 1;
    } else {
      edAddObject(OT_START, mx, my, 0, 0, 0, 0);
      edScreen(1);
    }
  } else {
    if (moving) {
      moving = 0;
      edScreen(1);
    }
    if (button == 3 && o != NULL) {
      edDelObject(o);
      edScreen(1);
    }
  }
}

void edMouseMotionObject(int mx, int my, int umx, int umy) {
  int i, dx, dy;
  object_t *o;
  int r, c;
  char *name = NULL;

  grBegin();
  grSetPixelMode(PMD_XOR);
  grSetColor(255);
  if (omx != -1 || omy != -1) edOctagon(omx, omy, 4);
  edOctagon(mx, my, 4);
  grSetPixelMode(PMD_SET);
  grEnd();
  if (moving) {
    if (so != NULL) {
      if (edGetObject(mx, my) == NULL) {
        grBegin();
        grSetColor(254);
        grSetPixelMode(PMD_XOR);
        edVertex(so->x, so->y);
        edVertex(mx, my);
        grSetPixelMode(PMD_SET);
        grEnd();
        so->x = mx;
        so->y = my;
      }
    }
  } else {
    o = NULL;
    for (i = 0; i < oc.n; ++i) {
      dx = mx - oc.p[i].x;
      dy = my - oc.p[i].y;
      oc.p[i].md = dx * dx + dy * dy;
      if (o == NULL || oc.p[i].md < o->md) o = oc.p + i;
    }
    if ((so != NULL || o != NULL) && o != so) {
      grBegin();
      if (so != NULL) {
        edGetObjectProperties(so->what, &r, &c);
        edObject(so->x, so->y, so->c, r, c);
      }
      if (o != NULL) {
        name = edGetObjectProperties(o->what, &r, &c);
        edObject(o->x, o->y, o->c, r, c);
      }
      grEnd();
      if (o != NULL) {
        edStBegin();
        grprintf("object #%d - %s - x:%d y:%d z:%d a:%d b:%d c:%d",
          o - oc.p, name, o->x, o->y, o->z, o->a, o->b, o->c);
        edStEnd();
      }
    }
    so = o;
  }
}

void edKeyboardObject(int key) {
  int r, c;
  char *name;
  
  if (so != NULL) {
    grBegin();
    edGetObjectProperties(so->what, &r, NULL);
    edObject(so->x, so->y, so->c, r, 0);
    switch (key) {
      case SDLK_1:
        if (so->what > 0) --so->what;
        break;
      case SDLK_2:
        if (so->what < OT_LAST-1) ++so->what;
        break;
      case SDLK_3:
        so->z -= 8;
        break;
      case SDLK_4:
        so->z += 8;
        break;
      case SDLK_5:
        so->a = (so->a - 1) & 7;
        break;
      case SDLK_6:
        so->a = (so->a + 1) & 7;
        break;
      case SDLK_7:
        so->b = (so->b - 1) & 7;
        break;
      case SDLK_8:
        so->b = (so->b + 1) & 7;
        break;
      case SDLK_9:
        so->c = (so->c - 1) & 7;
        break;
      case SDLK_0:
        so->c = (so->c + 1) & 7;
        break;
      default:
        break;
    }
    name = edGetObjectProperties(so->what, &r, &c);
    edObject(so->x, so->y, so->c, r, c);
    grEnd();
    edStBegin();
    grprintf("object #%d - %s - x:%d y:%d z:%d a:%d b:%d c:%d",
      so - oc.p, name, so->x, so->y, so->z, so->a, so->b, so->c);
    edStEnd();
  }
}

typedef struct {
  short type : 16;
  short x, y, z : 16;
  short rot : 16;
} __attribute__((packed)) fobject_t;

int edSaveObjects(FILE *fp) {
  fobject_t fo;
  object_t *o;
  int i;

  fwrite(&oc.n, sizeof(int), 1, fp);
  o = oc.p;
  for (i = oc.n; i; --i, ++o) {
    fo.type = o->what;
    fo.x = o->x;
    fo.y = o->y;
    fo.z = o->z;
    fo.rot = (o->a & 7) | ((o->b & 7) << 3) | ((o->c & 7) << 6);

    if (fwrite(&fo, sizeof(fobject_t), 1, fp) != 1) return 0;
  }
  return !0;
}
