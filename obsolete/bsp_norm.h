#ifndef _BSP_NORM_H
#define _BSP_NORM_H 1

void bspPutNormal(double x, double y, double z, double nx, double ny, double nz);
double *bspGetNormal(double x, double y, double z);
void bspFlushNormals();
int bspInitNorm();
void bspDoneNorm();
void bspDrawNormals(double x, double y, double z);

#endif /* _BSP_NORM_H */
