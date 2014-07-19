#ifndef _LINE_H
#define _LINE_H 1

typedef enum {
  LF_NOTHING = 0x00,
  LF_TWOSIDED = 0x01,
  LF_TOPSTART = 0x02,
  LF_BOTTOMSTART = 0x04,
  LF_BLOCK = 0x08
} lineflag_t;

typedef struct {
  int a, b;
  int sf, sb;
  int md;
  unsigned u, v;
  unsigned flags;
  unsigned tf, tb;
  int du;
} line_t;

typedef struct {
  line_t *p;
  unsigned alloc, n;
} lc_t;

extern lc_t lc;
extern line_t *sl, tmpline;

line_t *edGetLine(int a, int b);
line_t *edAddLine(int a, int b, int sf, int sb, int u, int v, int flags, int tf, int tb, int du);
void edDelLine(line_t *p);
void edMouseButtonLine(int mx, int my, int button);
void edMouseMotionLine(int mx, int my, int umx, int umy);
void edKeyboardLine(int key);

#endif /* _LINE_H */
