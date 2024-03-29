/* vim: set ts=2 sw=8 tw=0 et :*/
#ifndef _SECTOR_H
#define _SECTOR_H 1

#include <vector>
#include <istream>
#include <ostream>

struct Sector {
  short f;
  short c;
  unsigned l;
  short u;
  short v;
  unsigned t;

  Sector() : f(0), c(0), l(0), u(0), v(0), t(0) {}

  void saveText(std::ostream& os, size_t index) const;
  static Sector loadText(std::istream& is, size_t expectedIndex);
};

typedef std::vector<Sector> Sectors;

extern Sectors sc;
extern int ss;

int edGetSector(unsigned s);
int edAddSector(unsigned s, int f, int c, int l, int u, int v, int t);
void edDelSector(int s);
int edAllocSector();
void edSelectSector(int s, int o);
void edMouseButtonSector(int mx, int my, int button);
void edMouseMotionSector(int mx, int my, int umx, int umy);
void edKeyboardSector(int key);

#endif /* _SECTOR_H */
