#ifndef _BSP_H
#define _BSP_H 1

#include <ostream>

namespace bsp {

int bspInit();
void bspDone();
int bspAddLine(int sf, int sb, int x1, int y1, int x2, int y2, int u, int v, int flags, int t, int du);
void bspBuildTree();
void bspShow();
int bspSave(std::ostream& os);

} // namespace bsp

#endif /* _BSP_H */
