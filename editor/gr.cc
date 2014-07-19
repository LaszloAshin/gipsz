#include <string.h>
#include <stdarg.h>
#include <SDL/SDL.h>
#include "main.h"
#include "gr.h"
#include "fixed8.h"

#define ABS(a) (((a) < 0) ? (-(a)) : (a))
#define VPSTKDEP 4

static SDL_Surface *sf = NULL;
static rect_t ua, vps[VPSTKDEP], *vp = vps;
static unsigned grCurColor = 0;
static struct {
  int x, y;
} pos;

static void isw(int *a, int *b) {
  int c = *a;
  *a = *b;
  *b = c;
}

void grSetViewPort(int x1, int y1, int x2, int y2) {
  if (!x1 && !x2 && !y1 && !y2) {
    x2 = sf->w - 1;
    y2 = sf->h - 1;
  }
  if (x1 > x2) isw(&x1, &x2);
  if (y1 > y2) isw(&y1, &y2);
  if (x2 < 0 || y2 < 0 || x1 >= sf->w || y1 >= sf->h) return;
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= sf->w) x2 = sf->w - 1;
  if (y2 >= sf->h) y2 = sf->h - 1;
  vp->x1 = x1;
  vp->y1 = y1;
  vp->x2 = x2;
  vp->y2 = y2;
}

int grPushViewPort() {
  if (vp < vps + VPSTKDEP - 1) {
    ++vp;
    *vp = vp[-1];
    return !0;
  }
  return 0;
}

int grPopViewPort() {
  if (vp > vps) {
    --vp;
    return !0;
  }
  return 0;
}

static void grExtendUpdateArea(int x, int y) {
  if (x < ua.x1) ua.x1 = x;
  if (x > ua.x2) ua.x2 = x;
  if (y < ua.y1) ua.y1 = y;
  if (y > ua.y2) ua.y2 = y;
}

/* SetPixel */
static void grSetPixel8(unsigned offs) {
  char *p = (char *)sf->pixels + offs;
  *p = grCurColor;
}

static void grSetPixel16(unsigned offs) {
  short *p = (short *)sf->pixels + offs;
  *p = grCurColor;
}

static void grSetPixel24(unsigned offs) {
  char *p = (char *)sf->pixels + (offs * 3);
  *p = grCurColor & 0xffffff;
}

static void grSetPixel32(unsigned offs) {
  int *p = (int *)sf->pixels + offs;
  *p = grCurColor;
}

/* XorPixel */
static void grXorPixel8(unsigned offs) {
  char *p = (char *)sf->pixels + offs;
  *p ^= grCurColor;
}

static void grXorPixel16(unsigned offs) {
  short *p = (short *)sf->pixels + offs;
  *p ^= grCurColor;
}

static void grXorPixel24(unsigned offs) {
  char *p = (char *)sf->pixels + (offs * 3);
  *p ^= grCurColor & 0xffffff;
}

static void grXorPixel32(unsigned offs) {
  int *p = (int *)sf->pixels + offs;
  *p ^= grCurColor;
}

typedef void (*grDrawPixel_t)(unsigned offs);

static grDrawPixel_t grDrawPixels[4][PMD_LAST] = {
  { grSetPixel8 , grXorPixel8  },
  { grSetPixel16, grXorPixel16 },
  { grSetPixel24, grXorPixel24 },
  { grSetPixel32, grXorPixel32 },
};

static grDrawPixel_t grDrawPixel = grSetPixel8;

static pixelmode_t grPixelMode = PMD_SET;

void grSetPixelMode(pixelmode_t pm) {
  grPixelMode = pm;
//  if (grPixelMode < PMD_FIRST) grPixelMode = PMD_FIRST;
  if (grPixelMode >= PMD_LAST) grPixelMode = pixelmode_t(PMD_LAST - 1);
  if (sf == NULL) return;
  int nb = sf->format->BytesPerPixel - 1;
  if (nb < 0 || nb > 3) return;
  grDrawPixel = grDrawPixels[nb][grPixelMode];
}

void grPutPixel(int x, int y) {
  x += vp->x1;  y += vp->y1;
  if (sf == NULL || x < vp->x1 || y < vp->y1 || x > vp->x2 || y > vp->y2) return;
  grDrawPixel(y * sf->w + x);
  grExtendUpdateArea(x, y);
}

void grBar(int x1, int y1, int x2, int y2) {
  x1 += vp->x1;  y1 += vp->y1;
  x2 += vp->x1;  y2 += vp->y1;
  if (x1 > x2) isw(&x1, &x2);
  if (y1 > y2) isw(&y1, &y2);
  if (sf == NULL || x1 > vp->x2 || y1 > vp->y2 || x2 < vp->x1 || y2 < vp->y1) return;
  if (x1 < vp->x1) x1 = vp->x1;
  if (y1 < vp->y1) y1 = vp->y1;
  if (x2 > vp->x2) x2 = vp->x2;
  if (y2 > vp->y2) y2 = vp->y2;
  unsigned d1 = x2 - x1 + 1;
  unsigned d2 = sf->w - d1;
  unsigned offs = y1 * sf->w + x1;
  unsigned x, y;
  for (y = y2 - y1 + 1; y; --y) {
    for (x = d1; x; --x) grDrawPixel(offs++);
    offs += d2;
  }
  grExtendUpdateArea(x1, y1);
  grExtendUpdateArea(x2, y2);
}

void grHorizLine(int x1, int y, int x2) {
  x1 += vp->x1;
  y += vp->y1;
  x2 += vp->x1;
  if (x1 > x2) isw(&x1, &x2);
  if (sf == NULL || x2 < vp->x1 || x1 > vp->x2 || y < vp->y1 || y > vp->y2) return;
  if (x1 < vp->x1) x1 = vp->x1;
  if (x2 > vp->x2) x2 = vp->x2;
  unsigned offs = y * sf->w + x1;
  int x;
  for (x = x2 - x1 + 1; x; --x) grDrawPixel(offs++);
  grExtendUpdateArea(x1, y);
  grExtendUpdateArea(x2, y);
}

void grVertLine(int x, int y1, int y2) {
  x += vp->x1;
  y1 += vp->y1;
  y2 += vp->y1;
  if (y1 > y2) isw(&y1, &y2);
  if (sf == NULL || y2 < vp->y1 || y1 > vp->y2 || x < vp->x1 || x > vp->x2) return;
  if (y1 < vp->y1) y1 = vp->y1;
  if (y2 > vp->y2) y2 = vp->y2;
  unsigned offs = y1 * sf->w + x;
  int y;
  for (y = y2 - y1 + 1; y; --y) {
    grDrawPixel(offs);
    offs += sf->w;
  }
  grExtendUpdateArea(x, y1);
  grExtendUpdateArea(x, y2);
}

int grLine45(int x1, int y1, int x2, int y2) {
  x1 += vp->x1;  y1 += vp->y1;
  x2 += vp->x1;  y2 += vp->y1;
  if ((x1 + y1 != x2 + y2) && (x1 - y1 != x2 - y2)) return 0;
  if (x1 > x2) {
    isw(&x1, &x2);
    isw(&y1, &y2);
  }
  if (sf == NULL) return 1;
  if (y2 > y1) { /* lefele megy */
    if (x1 < vp->x1) {
      y1 += vp->x1 - x1;
      x1 = vp->x1;
    }
    if (y1 < vp->y1) {
      x1 += vp->y1 - y1;
      y1 = vp->y1;
    }
    if (x2 > vp->x2) {
      y2 -= x2 - vp->x2;
      x2 = vp->x2;
    }
    if (y2 > vp->y2) {
      x2 -= y2 - vp->y2;
      y2 = vp->y2;
    }
    if (y2 < vp->y1 || y1 > vp->y2 || x2 < vp->x1 || x1 > vp->x2) return 1;
    unsigned offs = y1 * sf->w + x1;
    int x;
    for (x = x2 - x1 + 1; x; --x) {
      grDrawPixel(offs);
      offs += sf->w + 1;
    }
  } else { /* felfele megy */
    if (x1 < vp->x1) {
      y1 -= vp->x1 - x1;
      x1 = vp->x1;
    }
    if (y1 > vp->y2) {
      x1 += y1 - vp->y2;
      y1 = vp->y2;
    }
    if (x2 > vp->x2) {
      y2 += x2 - vp->x2;
      x2 = vp->x2;
    }
    if (y2 < vp->y1) {
      x2 -= vp->y1 - y2;
      y2 = vp->y1;
    }
    if (y1 < vp->y1 || y2 > vp->y2 || x2 < vp->x1 || x1 > vp->x2) return 1;
    unsigned offs = y1 * sf->w + x1;
    int x;
    for (x = x2 - x1 + 1; x; --x) {
      grDrawPixel(offs);
      offs -= sf->w - 1;
    }
  }
  grExtendUpdateArea(x1, y1);
  grExtendUpdateArea(x2, y2);
  return 1;
}

void grLine(int x1, int y1, int x2, int y2) {
  if (y2 == y1) {
    grHorizLine(x1, y1, x2);
  } else if (x2 == x1) {
    grVertLine(x1, y1, y2);
  } else if (!grLine45(x1, y1, x2, y2)) {
    if (ABS(x2 - x1) > ABS(y2 - y1)) {
      if (x2 < x1) {
        isw(&x1, &x2);
        isw(&y1, &y2);
      }
      int y = y1 * 65536 + 32768, dy = 0;
      if (x1 != x2) dy = (y2 - y1) * 65536 / (x2 - x1);
      for (; x1 <= x2; ++x1, y += dy) grPutPixel(x1, y / 65536);
    } else {
      if (y2 < y1) {
        isw(&x1, &x2);
        isw(&y1, &y2);
      }
      int x = x1 * 65536 + 32768, dx = 0;
      if (y1 != y2) dx = (x2 - x1) * 65536 / (y2 - y1);
      for (; y1 <= y2; ++y1, x += dx) grPutPixel(x / 65536, y1);
    }
  }
}

void grLineTo(int x, int y) {
  grLine(pos.x, pos.y, x, y);
  grSetPos(x, y);
}

void grClear() {
  if (sf == NULL) return;
  grSetColor(0);
  grBar(vp->x1, vp->y1, vp->x2, vp->y2);
}

void grSetColor(int c) {
  grCurColor = c;
}

void grBegin() {
  if (sf == NULL) return;
  SDL_LockSurface(sf);
  ua.x1 = sf->w;
  ua.y1 = sf->h;
  ua.x2 = ua.y2 = 0;
}

void grEnd() {
  if (sf == NULL) return;
  SDL_UnlockSurface(sf);
  if (ua.x2 >= ua.x1 && ua.y2 >= ua.y1) {
    if (ua.x2 >= vp->x1 && ua.y2 >= vp->y1 && ua.x1 <= vp->x2 && ua.y1 <= vp->y2) {
      if (ua.x1 < vp->x1) ua.x1 = vp->x1;
      if (ua.y1 < vp->y1) ua.y1 = vp->y1;
      if (ua.x2 > vp->x2) ua.x2 = vp->x2;
      if (ua.y2 > vp->y2) ua.y2 = vp->y2;
      unsigned w = ua.x2 - ua.x1 + 1;
      unsigned h = ua.y2 - ua.y1 + 1;
      SDL_UpdateRect(sf, ua.x1, ua.y1, w, h);
    }
  }
}

int grSetSurface(SDL_Surface *s) {
  if (s == NULL) return 0;
  sf = s;
  grSetPixelMode(grPixelMode);
  grSetViewPort(0, 0, sf->w - 1, sf->h - 1);
  return !0;
}

void grSetPos(int x, int y) {
  pos.x = x;
  pos.y = y;
}

void grPutChar(char ch) {
  if (pos.x > -8 && pos.x < vp->x2-vp->x1 && pos.y > -16 && pos.y < vp->y2-vp->y1) {
    ch -= 32;
    if (ch < 0 || ch >= 96) ch = '?';
    int y;
    for (y = 0; y < 16; ++y) {
      int sor = fixed8[(unsigned)ch][y];
      int x;
      for (x = 0; x < 8; ++x)
        if ((sor << x) & 0x80)
          grPutPixel(pos.x + x, pos.y - y + 16);
    }
  }
  pos.x += 8;
}

void grPutStr(const char *s) {
  for (; *s; ++s) grPutChar(*s);
}

void grprintf(const char *s, ...) {
  va_list ap;
  int i, b;
  double f;
  char *p;
  char buf[33], buf2[33];
  va_start(ap, s);
  for (; *s; ++s) {
    if (*s != '%')
      grPutChar(*s);
    else {
      int n = 2;
     ide:
      switch (*++s) {
        case 'd':
        case 'x':
          i = va_arg(ap, int);
          b = (n+1 < 32) ? n+1 : 32;
          memcpy(buf2, s-n+1, b);
          buf2[b-1] = '\0';
          snprintf(buf, 32, buf2, i);
          buf[32] = '\0';
          grPutStr(buf);
          break;
        case 'f':
          f = va_arg(ap, double);
          b = (n+1 < 32) ? n+1 : 32;
          memcpy(buf2, s-n+1, b);
          buf2[b-1] = '\0';
          snprintf(buf, 32, buf2, f);
          buf[32] = '\0';
          grPutStr(buf);
          break;
        case 's':
          p = va_arg(ap, char*);
          grPutStr(p);
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.':
          if (!s[1]) {
            grPutChar(*s);
            break;
          }
          ++n;
          goto ide;
        case '%':
          grPutChar('%');
          break;
        default:
          grPutChar('%');
          grPutChar(*s);
          break;
      }
    }
  }
  va_end(ap);
}

void grOctagon(int x, int y, int a) {
  int a2 = a / 2;
  int c = a2 + 1000 * a / 1414;
  int x1 = x + a2;
  int y1 = y - c;
  int x2 = x + c;
  int y2 = y - a2;
  grLine(x1, y1, x2, y2);
  x1 = x2;
  y1 = y + a2;
  grLine(x1, y1, x2, y2);
  x2 = x + a2;
  y2 = y + c;
  grLine(x1, y1, x2, y2);
  x1 = x - a2;
  y1 = y2;
  grLine(x1, y1, x2, y2);
  x2 = x - c;
  y2 = y + a2;
  grLine(x1, y1, x2, y2);
  x1 = x2;
  y1 = y - a2;
  grLine(x1, y1, x2, y2);
  x2 = x - a2;
  y2 = y - c;
  grLine(x1, y1, x2, y2);
  x1 = x + a2;
  y1 = y2;
  grLine(x1, y1, x2, y2);
}

void grRectangle(int x1, int y1, int x2, int y2) {
  grHorizLine(x1, y1, x2);
  grHorizLine(x1, y2, x2);
  grVertLine(x1, y1, y2);
  grVertLine(x2, y1, y2);
}

void grAlignMouse(int *mx, int *my) {
  if (*mx < vp->x1) *mx = -1;
  if (*my < vp->y1) *my = -1;
  if (*mx > vp->x2) *mx = -1;
  if (*my > vp->y2) *my = -1;
  *mx -= vp->x1;
  *my -= vp->y1;
}
