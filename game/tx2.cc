#include <SDL/SDL_opengl.h>
#include "tx2.h"
#include "tex.h"
#include "tga.h"
#include "console.h"
#include "main.h"
#include "ogl.h"

static int colors[32] = {
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

static GLuint vw = 0, fw = 0;
static int chw[128];
static float cheight = 0.1, cwidth = 0.05;
static int fixed = 0;
static GLuint ls = 0;

int tx2Init() {
  tgaimage_t im;
  int i;
  float u, v, f;

  if (!vw) {
    if (!tgaRead(&im, "data/gfx/charset.tga")) {
      cmsg(MLERR, "tx2Init: unable to read charset");
      return 0;
    }
    if (im.width != im.height || im.width != 256 || im.bytes != 1) {
      cmsg(MLERR, "tx2Init: wrong charset dimensions");
      return 0;
    }
    glGenTextures(1, &vw);
    glBindTexture(GL_TEXTURE_2D, vw);
    texImage(0, GL_ALPHA4, im.width, im.height, GL_ALPHA, im.data, im.bytes);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    for (i = 0; i < 128; ++i) {
      int w;
      for (w = 16; w; --w) {
        unsigned char *p = im.data + ((i & 15) << 4) + ((i >> 4) << 13) + w - 1;
        int y;
        for (y = 32; y; --y, p += 256) if (*p) break;
        if (y) break;
      }
      chw[i] = w;
    }
    chw[32] = 8;
    tgaFree(&im);
  }
  if (!fw) {
    if (!tgaRead(&im, "data/gfx/charset-mono.tga")) {
      cmsg(MLERR, "tx2Init: unable to read charset");
      return 0;
    }
    if (im.width != im.height || im.width != 256 || im.bytes != 1) {
      cmsg(MLERR, "tx2Init: wrong charset dimensions");
      return 0;
    }
    glGenTextures(1, &fw);
    glBindTexture(GL_TEXTURE_2D, fw);
    texImage(0, GL_ALPHA4, im.width, im.height, GL_ALPHA, im.data, im.bytes);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    tgaFree(&im);
  }
  ls = glGenLists(128);
  if (!ls) return 0;
  f = 1.0 / 512.0;
  for (i = 0; i < 128; ++i) {
    u = (float)(i & 15) * 0.0625;
    v = (float)(i >> 4) * 0.125;
    glNewList(ls + i, GL_COMPILE);
    glBegin(GL_QUADS);
    glTexCoord2f(u, v + 0.125 - f);
    glVertex2f(0.0, 0.0);
    glTexCoord2f(u + 0.0625 - f, v + 0.125 - f);
    glVertex2f(1.0, 0.0);
    glTexCoord2f(u + 0.0625 - f, v + f);
    glVertex2f(1.0, 1.0);
    glTexCoord2f(u, v + f);
    glVertex2f(0.0, 1.0);
    glEnd();
    glEndList();
  }
  return !0;
}

void tx2Done() {
  if (ls) {
    glDeleteLists(ls, 128);
    ls = 0;
  }
  if (fw) {
    glDeleteTextures(1, &fw);
    fw = 0;
  }
  if (vw) {
    glDeleteTextures(1, &vw);
    vw = 0;
  }
}

void tx2PutStr(float x, float y, const char *s) {
  unsigned i;
  int c;

  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, fixed ? fw : vw);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(x, y, 0.0);
  glScalef(cwidth, cheight, 1.0);
  for (; *s; ++s) {
    i = *s;
    if (i < 32) {
      c = colors[i];
      glColor3ub(c, c >> 8, c >> 16);
    } else {
      glCallList(ls + i);
      glTranslatef(fixed ? 1.0 : 0.0625 * chw[i], 0.0, 0.0);
    }
  }
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glDisable(GL_ALPHA_TEST);
}

float tx2GetStrWidth(const char *s) {
  float w = 0.0;
  for (; *s; ++s) {
    unsigned i = *s;
    if (i >= 32) w += fixed ? cwidth : cwidth * chw[i] * 0.0625;
  }
  return w;
}

void tx2SetWidth(float w) { cwidth = w; }
void tx2SetHeight(float h) { cheight = h; }
float tx2GetWidth(void) { return cwidth; }
float tx2GetHeight(void) { return cheight; }

void tx2SetFixed(int f) {
  fixed = f;
  if (fixed) {
    cwidth = 16.0 / scrGetWidth();
    cheight = 32.0 / scrGetHeight();
  }
}
