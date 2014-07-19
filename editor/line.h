#ifndef _LINE_H
#define _LINE_H 1

#include <vector>

struct Line {
  struct Flag {
    enum Type {
      NOTHING = 0x00,
      TWOSIDED = 0x01,
      TOPSTART = 0x02,
      BOTTOMSTART = 0x04,
      BLOCK = 0x08
    };
  };

  int a, b;
  int sf, sb;
  int md;
  unsigned u, v;
  unsigned flags;
  unsigned tf, tb;
  int du;

  Line() : a(0), b(0), sf(0), sb(0), md(0), u(0), v(0), flags(0), tf(0), tb(0), du(0) {}
};

typedef std::vector<Line> Lines;

extern Lines lc;
extern Line* sl, tmpline; // XXX: tmpline wtf?!

Line* edGetLine(int a, int b);
Line* edAddLine(int a, int b, int sf, int sb, int u, int v, int flags, int tf, int tb, int du);
void edDelLine(Line* p);
void edMouseButtonLine(int mx, int my, int button);
void edMouseMotionLine(int mx, int my, int umx, int umy);
void edKeyboardLine(int key);

#endif /* _LINE_H */
