/* vim: set ts=2 sw=8 tw=0 et :*/
#include <stdlib.h>
#include <math.h>
#include "object.h"
#include "gr.h"
#include "ed.h"

Object
Object::load(std::istream& is)
{
  Object result;
  char buf[5 * 2], *p = buf;
  is.read(buf, sizeof(buf));
  result.what = ObjType::Type(*(short *)p);
  result.x = *(short *)(p + 2);
  result.y = *(short *)(p + 4);
  result.z = *(short *)(p + 6);
  const short rot = *(short *)(p + 8);
  result.a = rot & 7;
  result.b = (rot >> 3) & 7;
  result.c = (rot >> 6) & 7;
  return result;
}

void
Object::save(std::ostream& os)
const
{
  char buf[5 * 2], *p = buf;
  *(short *)p = what;
  *(short *)(p + 2) = x;
  *(short *)(p + 4) = y;
  *(short *)(p + 6) = z;
  *(short *)(p + 8) = (a & 7) | ((b & 7) << 3) | ((c & 7) << 6);
  os.write(buf, sizeof(buf));
}

void
Object::print(std::ostream& os, unsigned index)
const
{
  os << "object #" << index;
  os << ": t=" << what;
  os << ", x=" << x;
  os << ", y=" << y;
  os << ", z=" << z;
  os << ", a=" << a;
  os << ", b=" << b;
  os << ", c=" << c;
  os << std::endl;
}


Objects oc;

Object* so = 0;

static const ObjProp objprop[] = {
  { ObjType::START, "start", 16, 255 },
  { ObjType::TUBE, "tube", 8, 250 },
  { ObjType::ELBOW, "elbow", 8, 250 },
  { ObjType::START, NULL, 0, 0 }
};

const char*
edGetObjectProperties(ObjType::Type wh, int* r, int* c)
{
  int i;

  for (i = 0; objprop[i].name != NULL; ++i)
    if (objprop[i].typ == wh) {
      if (r != NULL) *r = objprop[i].r;
      if (c != NULL) *c = objprop[i].c;
      return objprop[i].name;
    }
  return NULL;
}

Object*
edGetObject(int x, int y)
{
  for (unsigned i = 0; i < oc.size(); ++i)
    if (oc[i].x == x && oc[i].y == y)
      return &oc.at(i);
  return 0;
}

int
edAddObject(ObjType::Type wh, int x, int y, int z, int a, int b, int c)
{
  Object o;
  o.x = x;
  o.y = y;
  o.z = z;
  o.a = a;
  o.b = b;
  o.c = c;
  o.what = wh;
  oc.push_back(o);
  return oc.size() - 1;
}

void
edDelObject(Object* o) {
  unsigned i = o - &oc.front();
  if (i >= oc.size()) return;
  oc[i] = oc.back();
  oc.pop_back();
}

void edMouseButtonObject(int mx, int my, int button) {
  Object* o = edGetObject(mx, my);
  if (button == 1) {
    if (o != NULL) {
      moving = 1;
    } else {
      edAddObject(ObjType::START, mx, my, 0, 0, 0, 0);
      edScreen();
    }
  } else {
    if (moving) {
      moving = 0;
      edScreen();
    }
    if (button == 3 && o != NULL) {
      edDelObject(o);
      edScreen();
    }
  }
}

void edMouseMotionObject(int mx, int my, int umx, int umy) {
  (void)umx;
  (void)umy;
  int dx, dy;
  Object* o;
  int r = 8, c = 0;
  const char *name = NULL;

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
    for (unsigned i = 0; i < oc.size(); ++i) {
      dx = mx - oc[i].x;
      dy = my - oc[i].y;
      oc[i].md = dx * dx + dy * dy;
      if (o == NULL || oc[i].md < o->md) o = &oc.at(i);
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
          o - &oc.front(), name, o->x, o->y, o->z, o->a, o->b, o->c);
        edStEnd();
      }
    }
    so = o;
  }
}

void edKeyboardObject(int key) {
  int r = 8, c = 0;
  const char *name;
  
  if (so != NULL) {
    grBegin();
    edGetObjectProperties(so->what, &r, NULL);
    edObject(so->x, so->y, so->c, r, 0);
    switch (key) {
      case SDLK_1:
        if (so->what > 0) so->what = ObjType::Type(so->what - 1);
        break;
      case SDLK_2:
        if (so->what < ObjType::LAST - 1) so->what = ObjType::Type(so->what + 1);
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
      so - &oc.front(), name, so->x, so->y, so->z, so->a, so->b, so->c);
    edStEnd();
  }
}

static int
edSaveObject(FILE *fp, Object* o)
{
  unsigned char buf[2 + 3 * 2 + 2], *p = buf;
  *(short *)p = o->what;
  *(short *)(p + 2) = o->x;
  *(short *)(p + 4) = o->y;
  *(short *)(p + 6) = o->z;
  *(short *)(p + 8) = (o->a & 7) | ((o->b & 7) << 3) | ((o->c & 7) << 6);
  return (fwrite(buf, 1, sizeof(buf), fp) == sizeof(buf)) ? 0 : -1;
}

int edSaveObjects(FILE *fp) {
  unsigned n = oc.size();
  fwrite(&n, sizeof(unsigned), 1, fp);
  for (Objects::iterator i(oc.begin()); i != oc.end(); ++i) {
    if (edSaveObject(fp, &*i)) return 0;
  }
  return !0;
}
