/* vim: set ts=2 sw=8 tw=0 et :*/
#ifndef _OBJECT_H
#define _OBJECT_H 1

#include <vector>
#include <cstdio>

struct ObjType {
  enum Type {
    START,
    TUBE,
    ELBOW,
    LAST
  };
};

struct ObjProp {
  ObjType::Type typ;
  const char *name;
  int r; /* radius */
  int c; /* color */
};

struct Object {
  ObjType::Type what;
  int x, y, z;
  int a, b, c;
  int md;

  Object() : what(ObjType::START), x(0), y(0), z(0), a(0), b(0), c(0), md(0) {}
};

typedef std::vector<Object> Objects;

extern Objects oc;
extern Object* so;

const char* edGetObjectProperties(ObjType::Type wh, int* r, int* c);
Object* edGetObject(int x, int y);
int edAddObject(ObjType::Type wh, int x, int y, int z, int a, int b, int c);
void edDelObject(Object* o);
void edMouseButtonObject(int mx, int my, int button);
void edMouseMotionObject(int mx, int my, int umx, int umy);
void edKeyboardObject(int key);
int edSaveObjects(FILE *fp);

#endif /* _OBJECT_H */
