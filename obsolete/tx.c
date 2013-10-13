/**
 * tx.cpp
 * Laszlo Ashin 2006
 */

#include <string.h>
#include <SDL/SDL_opengl.h>

#include "main.h"
#include "tx.h"
#include "data/fixed8.h"

static char fixed8b[96][16];

#define CHW 32
#define CHH 64

#include "tx_impact.h"

typedef struct {
  unsigned width, color;
  unsigned data[CHH][CHW];
} char_t;

static char_t chars[96];
static unsigned colors[32] = {
  0x000000, /* 00 black */
  0xff0000, /* 01 blue */
  0x00ff00, /* 02 green */
  0xffff00, /* 03 cyan */
  0x0000ff, /* 04 red */
  0x8000ff, /* 05 magenta */
  0x0080ff, /* 06 brown */
  0xc0c0c0, /* 07 lightgray */
  0x808080, /* 10 darkgray */
  0xffa0a0, /* 11 lightblue */
  0x80ffa0, /* 12 lightgreen */
  0xffffa0, /* 13 lightcyan */
  0xa0a0ff, /* 14 lightred */
  0xffa0ff, /* 15 lightmagenta */
  0x00ffff, /* 16 yellow */
  0xffffff, /* 17 white */
};

static float curXFactor = 1.0, curYFactor = 1.0;
static unsigned curColor = 0xffffffff;
static textfilter_t curFilter = TXF_KEEP;

static void txInitFixed8b() {
  int ch, x, y;
//  memcpy(fixed8b, fixed8, sizeof(fixed8));
  memset(fixed8b, 0, sizeof(fixed8b));
  for (ch = 0; ch < 96; ++ch)
    for (y = 0; y < 16; ++y)
      for (x = 0; x < 8; ++x) {
        if (fixed8[ch][y] & (1 << x)) {
          if (x > 0) fixed8b[ch][y] |= 1 << (x - 1);
          if (x < 7) fixed8b[ch][y] |= 1 << (x + 1);
          if (y > 0) fixed8b[ch][y-1] |= 1 << x;
          if (y < 15) fixed8b[ch][y+1] |= 1 << x;
        }
      }
}

void txInit() {
  memset(chars, 0, sizeof(chars));
  txInitFixed8b();
}

void txDone() {
  memset(chars, 0, sizeof(chars));
}

void txSetHeight(float h) {
  if (h < 0.0 || h > 1.0) return;
  curXFactor = curYFactor = h * scrGetHeight() / 64;
}

void txSetWidth(float h) {
  if (h < 0.0 || h > 1.0) return;
  curXFactor = curYFactor = h * scrGetWidth() / 32;
}

float txGetHeight() {
  return (8.0 + CHH) * curYFactor / scrGetHeight();
}

void txSetColor(unsigned c) {
  curColor = c;
}

static char_t *txGetChar(unsigned char ch) {
  ch -= 32;
  if (ch >= 96) return NULL;
  if (chars[ch].color == curColor)
    return &chars[ch];
  unsigned x, y;
  unsigned p[CHH][CHW];
  for (y = 0; y < CHH; ++y)
    for (x = 0; x < CHW; ++x) {
      p[y][x] = ((map[ch][CHH-1 - y] >> x) & 1) ? curColor : 0;
    }
  for (y = 0; y < CHH; ++y) {
    p[y][0] &= 0xff000000;
    p[y][CHW-1] &= 0xff000000;
  }
  for (x = 1; x < CHW-1; ++x) {
    p[0][x] &= 0xff000000;
    p[CHH-1][x] &= 0xff000000;
  }
  for (y = 1; y < CHH-1; ++y)
    for (x = 1; x < CHW-1; ++x)
      if (p[y][x] & 0x00ffffff) {
        p[y][x-1] |= 0xff000000;
        p[y][x+1] |= 0xff000000;
        p[y-1][x] |= 0xff000000;
        p[y+1][x] |= 0xff000000;
      }
  int i;
  for (i = 0; i < 1; ++i) {
    unsigned avg;
    for (y = 1; y < CHH-1; ++y)
      for (x = 1; x < CHW-1; ++x) {
        if (!p[y][x]) {
          chars[ch].data[y][x] = 0;
          continue;
        }
        chars[ch].data[y][x] = 0xff000000;

        avg = (p[y+1][x+1] >> 16) & 0xff;
        avg += (p[y+1][x-1] >> 16) & 0xff;
        avg += (p[y-1][x+1] >> 16) & 0xff;
        avg += (p[y-1][x-1] >> 16) & 0xff;
        avg += (p[y][x+1] >> 16) & 0xff;
        avg += (p[y][x-1] >> 16) & 0xff;
        avg += (p[y+1][x] >> 16) & 0xff;
        avg += (p[y-1][x] >> 16) & 0xff;
        chars[ch].data[y][x] |= (avg << 13) & 0x00ff0000;

        avg = (p[y+1][x+1] >> 8) & 0xff;
        avg += (p[y+1][x-1] >> 8) & 0xff;
        avg += (p[y-1][x+1] >> 8) & 0xff;
        avg += (p[y-1][x-1] >> 8) & 0xff;
        avg += (p[y][x+1] >> 8) & 0xff;
        avg += (p[y][x-1] >> 8) & 0xff;
        avg += (p[y+1][x] >> 8) & 0xff;
        avg += (p[y-1][x] >> 8) & 0xff;
        chars[ch].data[y][x] |= (avg << 5) & 0x0000ff00;

        avg = p[y+1][x+1] & 0xff;
        avg += p[y+1][x-1] & 0xff;
        avg += p[y-1][x+1] & 0xff;
        avg += p[y-1][x-1] & 0xff;
        avg += p[y][x+1] & 0xff;
        avg += p[y][x-1] & 0xff;
        avg += p[y+1][x] & 0xff;
        avg += p[y-1][x] & 0xff;
        chars[ch].data[y][x] |= (avg >> 3) & 0x000000ff;
      }
    for (y = 0; y < CHH; ++y)
      for (x = 0; x < CHW; ++x)
        p[y][x] = chars[ch].data[y][x];
  }
  chars[ch].width = CHW;
  if (ch) {
    while (chars[ch].width) {
      x = 0;
      for (y = 0; y < CHH; ++y)
        if (p[y][chars[ch].width-1]) {
          ++x;
          break;
        }
      if (x) break;
      --chars[ch].width;
    }
  } else
    chars[ch].width = CHW / 3;
  chars[ch].color = curColor;
  return &chars[ch];
}

static char txFilterFunc(char ch) {
  if (curFilter & TXF_UPPERCASE) {
    if (ch >= 'a' && ch <= 'z') ch -= 32;
  } else if (curFilter & TXF_LOWERCASE) {
    if (ch >= 'A' && ch <= 'Z') ch += 32;
  }
  return ch;
}

void txPutChar(float x, float y, unsigned char ch) {
  if (ch < 32 || ch > 128) ch = '?';
  glRasterPos2f(x, y);
  glAlphaFunc(GL_GREATER, 0.0);
  glPixelZoom(curXFactor, curYFactor);
  char_t *p = txGetChar(txFilterFunc(ch));
  if (p == NULL) return;
  glDrawPixels(CHW, CHH, GL_RGBA, GL_UNSIGNED_BYTE, p->data);
}

void txPutStr(float x, float y, const char *p) {
  glAlphaFunc(GL_GREATER, 0.0);
  glPixelZoom(curXFactor, curYFactor);
  for (; *p; ++p) {
    if ((unsigned)*p < 32) {
      txSetColor(colors[(unsigned)*p]);
    } else {
      glRasterPos2f(x, y);
      char_t *ch = txGetChar(txFilterFunc(*p));
      glDrawPixels(CHW, CHH, GL_RGBA, GL_UNSIGNED_BYTE, ch->data);
      x += 2.0 * ch->width * curXFactor / scrGetWidth();
    }
  }
}

static int bm[16];

void txPutChar8(float x, float y, unsigned char ch) {
  int i;
  if (ch < 32 || ch > 128) ch = '?';
  ch -= 32;
  glAlphaFunc(GL_ALWAYS, 0.0);
  glRasterPos2f(x, y);
  glColor3i(0, 0, 0);
  for (i = 0; i < 16; ++i) bm[i] = fixed8b[ch][i];
  glBitmap(8, 16, 0.0, 0.0, 0.0, 0.0, (GLubyte *)bm);
  glRasterPos2f(x, y);
  glColor3ub(curColor, curColor >> 8, curColor >> 16);
  for (i = 0; i < 16; ++i) bm[i] = fixed8[ch][i];
  glBitmap(8, 16, 0.0, 0.0, 0.0, 0.0, (GLubyte *)bm);
}

void txPutStr8(float x, float y, const char *s) {
  unsigned c = curColor;
  const char *p;
  unsigned char ch;
  int i;
  glAlphaFunc(GL_ALWAYS, 0.0);
  glColor3d(0.0, 0.0, 0.0);
  glRasterPos2f(x, y);
  for (p = s; *p; ++p) {
    if ((unsigned)*p >= 32) {
      ch = *p;
      if (ch < 32 || ch > 128) ch = '?';
      ch -= 32;
      for (i = 0; i < 16; ++i) bm[i] = fixed8b[ch][i];
      glBitmap(8, 16, 0.0, 0.0, 8.0, 0.0, (GLubyte *)bm);
    }
  }
  glColor3ub(curColor, curColor >> 8, curColor >> 16);
  glRasterPos2f(x, y);
  for (p = s; *p; ++p) {
    if ((unsigned)*p < 32) {
      c = colors[(unsigned)*p];
      glColor3ub(c, c >> 8, c >> 16);
    } else {
      ch = *p;
      if (ch < 32 || ch > 128) ch = '?';
      ch -= 32;
      for (i = 0; i < 16; ++i) bm[i] = fixed8[ch][i];
      glBitmap(8, 16, 0.0, 0.0, 8.0, 0.0, (GLubyte *)bm);
    }
  }
}

float txGetStrWidth(const char *p) {
  float w, x = 0;
  w = 2.0 * curXFactor / scrGetWidth();
  for (; *p; ++p)
    if ((unsigned)*p >= 32) {
      char_t *ch = txGetChar(txFilterFunc(*p));
      x += w * ch->width;
    }
  return x;
}

void txSetFilter(textfilter_t filter) {
  curFilter = filter;
}
