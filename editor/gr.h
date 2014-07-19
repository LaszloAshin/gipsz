#ifndef _GR_H
#define _GR_H 1

#include <SDL/SDL.h>

typedef struct {
  int x1, y1, x2, y2;
} rect_t;

typedef enum {
  PMD_FIRST = 0,
  PMD_SET = 0,
  PMD_XOR,
  PMD_LAST
} pixelmode_t;

int grSetSurface(SDL_Surface *s);

void grBegin();
void grEnd();

void grPutPixel(int x, int y);
void grBar(int x1, int y1, int x2, int y2);
void grLine(int x1, int y1, int x2, int y2);
void grLineTo(int x, int y);
void grClear();
void grSetColor(int c);
#define grMoveTo(x, y) grSetPos(x, y)
void grSetPos(int x, int y);
void grPutChar(char ch);
void grPutStr(const char *s);
void grprintf(const char *s, ...);
void grOctagon(int x, int y, int a);
void grRectangle(int x1, int y1, int x2, int y2);
void grSetViewPort(int x1, int y1, int x2, int y2);
void grAlignMouse(int *mx, int *my);
void grSetPixelMode(pixelmode_t pm);
int grPushViewPort();
int grPopViewPort();

#endif /* _GR_H */
