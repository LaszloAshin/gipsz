#include <stdio.h>
#include <string.h>
#include <SDL/SDL_opengl.h>
#include "tex.h"
#include "tga.h"
#include "console.h"
#include "mm.h"
#include "cmd.h"
#include "main.h"

typedef struct {
  unsigned id;
  GLuint textureName;
} tex_t;

static struct {
  tex_t *p;
  unsigned alloc, n;
} tc;

static int selected = -1;
static const cvlistitem_t texFilterList[] = {
  { GL_NEAREST, "GL_NEAREST" },
  { GL_LINEAR, "GL_LINEAR" },
  { GL_NEAREST_MIPMAP_NEAREST, "GL_NEAREST_MIPMAP_NEAREST" },
  { GL_NEAREST_MIPMAP_LINEAR, "GL_NEAREST_MIPMAP_LINEAR" },
  { GL_LINEAR_MIPMAP_NEAREST, "GL_LINEAR_MIPMAP_NEAREST" },
  { GL_LINEAR_MIPMAP_LINEAR, "GL_LINEAR_MIPMAP_LINEAR" },
  { 0, NULL }
};
static unsigned texFilter = GL_LINEAR_MIPMAP_LINEAR;
static const cvlistitem_t texDetailList[] = {
  { 512, "SUPER" },
  { 128, "HIGH" },
  { 64, "MEDIUM" },
  { 32, "LOW" },
  { 16, "UGLY" },
  { 0, NULL }
};
static int texDetail = 512;
static int texReload = 0;

static void texSetGLFilters(int min) {
  int mag;

  switch (min) {
    case GL_NEAREST_MIPMAP_NEAREST:
    case GL_NEAREST_MIPMAP_LINEAR:
      mag = GL_NEAREST;
      break;
    case GL_LINEAR_MIPMAP_NEAREST:
    case GL_LINEAR_MIPMAP_LINEAR:
      mag = GL_LINEAR;
      break;
    default:
      mag = min;
      break;
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
}

static void texFilterChange(void *addr) {
  (void)addr;
  for (unsigned i = 0; texFilterList[i].name != NULL; ++i)
    if (texFilterList[i].val == texFilter) {
      cmsg(MLHINT, "setting texture filter %s", texFilterList[i].name);
      break;
    }
  for (unsigned i = 0; i < tc.n; ++i) {
    if (!tc.p[i].textureName) continue;
    glBindTexture(GL_TEXTURE_2D, tc.p[i].textureName);
    texSetGLFilters(texFilter);
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  selected = 0xffffffff;
}

static unsigned texGetPos(unsigned id) {
  int m = 0, l = 0, r = tc.n - 1;
  while (l <= r) {
    m = (l + r) / 2;
    if (tc.p[m].id == id) return m;
    if (tc.p[m].id > id)
      r = m - 1;
    else
      l = m + 1;
  }
  if (r < 0) r = 0;
  for (; r < (int)tc.n && tc.p[r].id < id; ++r);
  return r;
}

static int texGetExactPos(unsigned id) {
  int m = 0, l = 0, r = tc.n - 1;
  while (l <= r) {
    m = (l + r) / 2;
    if (tc.p[m].id == id) return m;
    if (tc.p[m].id > id)
      r = m - 1;
    else
      l = m + 1;
  }
  return -1;
}

static int texAdd(unsigned id) {
  unsigned i;
  unsigned p = texGetPos(id);
  if (p < tc.n && tc.p[p].id == id) return p;
  if (tc.n == tc.alloc) {
    int na = tc.alloc * 2;
    tex_t *p = (tex_t *)mmAlloc(na * sizeof(tex_t));
    if (p == NULL) return -1;
    tc.alloc = na;
    for (i = 0; i < tc.n; ++i) p[i] = tc.p[i];
    mmFree(tc.p);
    tc.p = p;
  }
  for (i = tc.n; i > p; --i) tc.p[i] = tc.p[i - 1];
  tc.p[i].id = id;
  tc.p[i].textureName = 0;
  ++tc.n;
  return i;
}

void texFreeTexture(unsigned id) {
  int j, i = texGetExactPos(id);
  if (i < 0) return;
  if (tc.p[i].textureName) {
    glDeleteTextures(1, &tc.p[i].textureName);
    tc.p[i].textureName = 0;
  }
  for (j = tc.n-1; j > i; --j) tc.p[j] = tc.p[j - 1];
  --tc.n;
}

void texImage(int lev, int intfmt, GLsizei w, GLsizei h, GLenum fmt, void *data, int b) {
  unsigned char *p;
  int ls, n, y;

  p = static_cast<unsigned char*>(data);
  ls = w * b;
  n = (b > 2) ? 8 : 4;
  if (ls < n && h > 1) {
    p = static_cast<unsigned char*>(mmAlloc(h * n));
    if (p == NULL) {
      p = static_cast<unsigned char*>(data);
    } else {
      unsigned char* d = p;
      unsigned char* s = static_cast<unsigned char*>(data);
      for (y = h; y; --y) {
        memcpy(d, s, ls);
        d += n;
        s += ls;
      }
    }
  }
  glTexImage2D(GL_TEXTURE_2D, lev, intfmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, p);
  if (p != data) mmFree(p);
}

int texLoadTexture(unsigned id, int force) {
  if (!id) return !0;
  int k, i = texAdd(id);
  if (i < 0) return 0;
  if (tc.p[i].textureName) {
    if (!force) return !0;
  } else {
    glGenTextures(1, &tc.p[i].textureName);
  }
  cmsg(MLHINT, "Loading texture %04x", id);
  char fname[64];
  tgaimage_t ti, ti2;
  snprintf(fname, 64, "data/textures/tex%04x.tga", id);
  if (!tgaRead(&ti, fname)) {
    cmsg(MLERR, "unable to read image from file %s", fname);
    texFreeTexture(id);
    return 0;
  }
  unsigned w, h;
  GLint internalFormat;
  GLenum format;
  w = h = texDetail;
  switch (ti.bytes) {
    case 1:
      internalFormat = GL_LUMINANCE4;
      format = GL_LUMINANCE;
      break;
    case 2:
      internalFormat = GL_LUMINANCE6_ALPHA2;
      format = GL_LUMINANCE_ALPHA;
      break;
    case 3:
      internalFormat = GL_R3_G3_B2;
      format = GL_RGB;
      break;
    case 4:
      internalFormat = GL_RGB5_A1;
      format = GL_RGBA;
      break;
    default:
      cmsg(MLERR, "texLoadTexture: unsupported format");
      return 0;
  }
  if (ti.width > ti.height) {
    if (w > ti.width) w = ti.width;
    h = w * ti.height / ti.width;
    if (h > ti.height) h = ti.height; /* ez igencsak valoszinutlen :] */
  } else {
    if (h > ti.height) h = ti.height;
    w = h * ti.width / ti.height;
    if (w > ti.width) w = ti.width; /* ez igencsak valoszinutlen :] */
  }
  if (w != ti.width || h != ti.height) {
    cmsg(MLDBG, " Resize %dx%d to %dx%d", ti.width, ti.height, w, h);
    ti2.width = w;
    ti2.height = h;
    ti2.bytes = ti.bytes;
    ti2.data = NULL;
    if (!tgaScale(&ti2, &ti)) {
      tgaFree(&ti);
      texFreeTexture(id);
      return 0;
    }
    tgaFree(&ti);
    ti = ti2;
  } else {
    cmsg(MLDBG, " Full size: %dx%dx%d", w, h, ti.bytes);
  }
  glBindTexture(GL_TEXTURE_2D, tc.p[i].textureName);
  for (k = 0;;) {
    texImage(k, internalFormat, ti.width, ti.height, format, ti.data, ti.bytes);
    ++k;
    ti2.width = ti.width >> 1;
    ti2.height = ti.height >> 1;
    if (!ti2.width || !ti2.height) break;
    ti2.bytes = ti.bytes;
    ti2.data = NULL;
    if (!tgaScale(&ti2, &ti)) {
      tgaFree(&ti);
      texFreeTexture(id);
      return 0;
    }
//    cmsg(MLDBG, " Level %d size: %dx%d", k, ti2.width, ti2.height);
    tgaFree(&ti);
    ti = ti2;
  }
  tgaFree(&ti);
  texSetGLFilters(texFilter);
  return !0;
}

int texSelectTexture(int id) {
  if (id == selected) return !0;
  selected = id;
  if (!id) {
    glBindTexture(GL_TEXTURE_2D, 0);
    return !0;
  }
  int i = texGetExactPos(id);
  if (i < 0) {
    cmsg(MLERR, "texSelecTexture: unable to find texture #%d", id);
    return 0;
  }
  glBindTexture(GL_TEXTURE_2D, tc.p[i].textureName);
  return !0;
}

void texUpdate() {
  if (texReload) {
    glBindTexture(GL_TEXTURE_2D, 0);
    for (unsigned i = 0; i < tc.n; ++i) texLoadTexture(tc.p[i].id, !0);
/*    for (i = 0; i < tc.n; ++i) printf(" %d-%d", tc.p[i].id, tc.p[i].textureName);
    printf("\n");*/
    texReload = 0;
  }
  selected = -1;
}

static void texDetailChange(void *addr) {
  (void)addr;
  texReload = 1;
}

int texInit() {
  tc.alloc = 8;
  tc.p = (tex_t *)mmAlloc(tc.alloc * sizeof(tex_t));
  tc.n = 0;
  cmdAddList("texFilter", (int *)&texFilter, texFilterList, 1);
  cmdSetAccessFuncs("texFilter", NULL, texFilterChange);
  cmdAddList("texDetail", &texDetail, texDetailList, 1);
  cmdSetAccessFuncs("texDetail", NULL, texDetailChange);
  return !0;
}

void texFlush() {
  cmsg(MLDBG, "texFlush");
  unsigned i;
  glBindTexture(GL_TEXTURE_2D, 0);
  for (i = 0; i < tc.n; ++i)
    if (tc.p[i].textureName) {
      glDeleteTextures(1, &tc.p[i].textureName);
      tc.p[i].textureName = 0;
    }
  tc.n = 0;
}

void texDone() {
  cmsg(MLDBG, "texDone");
  texFlush();
  mmFree(tc.p);
  tc.alloc = 0;
}

void texLoadReady() {
  static const unsigned char data = 0xff;
  glBindTexture(GL_TEXTURE_2D, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 1, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &data);
  texSetGLFilters(GL_NEAREST);
}

int texGetNofTextures() {
  return tc.n;
}
