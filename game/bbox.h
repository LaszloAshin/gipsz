#ifndef _BBOX_H
#define _BBOX_H 1

typedef struct {
  double x1, y1, z1;
  double x2, y2, z2;
} bbox_t;

void bbAdd(bbox_t *bb, int x, int y, int z);
int bbVisible(bbox_t *bb);
int bbInside(bbox_t *bb, double x, double y, double z, double b);

#endif /* _BBOX_H */
