#ifndef _SECTOR_H
#define _SECTOR_H 1

typedef struct {
  short f;
  short c;
  unsigned char l;
  short u;
  short v;
  unsigned t;
} sector_t;

typedef struct {
  sector_t *p;
  unsigned alloc;
} sc_t;

extern sc_t sc;
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
