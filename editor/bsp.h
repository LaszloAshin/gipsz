#ifndef _BSP_H
#define _BSP_H 1

int bspInit();
void bspDone();
int bspAddLine(int s, int x1, int y1, int x2, int y2, int u, int v, int flags, int t, int du);
void bspBuildTree();
void bspShow();
int bspSave(FILE *f);

#endif /* _BSP_H */
