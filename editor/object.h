#ifndef _OBJECT_H
#define _OBJECT_H 1

#include <stdio.h>

typedef enum {
  OT_START,
  OT_TUBE,
  OT_ELBOW,
  OT_LAST
} objtype_t;

typedef struct {
  objtype_t typ;
  const char *name;
  int r; /* radius */
  int c; /* color */
} objprop_t;

typedef struct {
  objtype_t what;
  int x, y, z;
  int a, b, c;
  int md;
} object_t;

typedef struct {
  object_t *p;
  unsigned alloc, n;
} oc_t;

extern oc_t oc;
extern object_t *so;

const char *edGetObjectProperties(objtype_t wh, int *r, int *c);
object_t *edGetObject(int x, int y);
int edAddObject(objtype_t wh, int x, int y, int z, int a, int b, int c);
void edDelObject(object_t *o);
void edMouseButtonObject(int mx, int my, int button);
void edMouseMotionObject(int mx, int my, int umx, int umy);
void edKeyboardObject(int key);
int edSaveObjects(FILE *fp);

#endif /* _OBJECT_H */
