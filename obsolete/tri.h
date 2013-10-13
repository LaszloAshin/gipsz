#ifndef _TRI_H
#define _TRI_H 1

typedef struct {
  double u, v;
} tritc_t;

typedef struct {
  double x, y, z;
} tri3dv_t;

typedef struct {
  double r, g, b;
} trirgb_t;

typedef struct tri_s {
  tri3dv_t v[3];
  tri3dv_t n;
  tritc_t tc[3];
  int tex;
  trirgb_t c[3];
} tri_t;

int triAdd(tri_t *tri);
int triBegin();
int triEnd();
void triFree();
int triHowMany(void);

#endif /* _TRI_H */
