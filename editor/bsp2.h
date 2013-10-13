#ifndef _BSP_H
#define _BSP_H 1

int bspInit();
void bspDone();
void bspAddLine(unsigned s, int x1, int y1, int x2, int y2);
void bspBuildTree();
void bspShow();

#endif /* _BSP_H */
