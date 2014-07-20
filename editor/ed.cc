/* vim: set ts=2 sw=8 tw=0 et :*/
#include <stdlib.h>
#include <math.h>
#include "ed.h"
#include "main.h"
#include "gr.h"
#include "bsp.h"
#include "st.h"
//#include "st2.h"
#include "vertex.h"
#include "line.h"
#include "sector.h"
#include "object.h"

static enum Mode {
  MD_FIRST = 0,
  MD_VERTEX = 0,
  MD_LINE,
  MD_SECTOR,
  MD_OBJECT,
  MD_LAST,
  MD_BSP
} mode = MD_VERTEX;

static const char *mdnames[] = {
  "vertex", "line", "sector", "object", "ILLEGAL", "bsp"
};

int moving = 0;
int omx = -1, omy = -1;
static int zoom = 4, sx = -10, sy = -10;
static rect_t tv;
int gs = 32;
int snap = 1;
static int grid = 1;

static void tr(int *x, int *y) {
  int dx = (tv.x2 - tv.x1) / 2;
  int dy = (tv.y2 - tv.y1) / 2;
  *x = (*x - sx - dx) * zoom / 10 + dx;
  *y = -(*y - sy - dy) * zoom / 10 + dy;
}

static void itr(int *x, int *y) {
  int dx = (tv.x2 - tv.x1) / 2;
  int dy = (tv.y2 - tv.y1) / 2;
  *x = (*x - dx) * 10 / zoom + dx + sx;
  *y = -(*y - dy) * 10 / zoom + dy + sy;
}

void edVertex(int x, int y) {
  tr(&x, &y);
  int a = 1000 * 2 / 586;
  grOctagon(x, y, a);
  a /= 2;
  grBar(x-a, y-a, x+a, y+a);
}

void edVector(int x1, int y1, int x2, int y2) {
  tr(&x1, &y1);
  tr(&x2, &y2);
  int dx = x2 - x1;
  int dy = y2 - y1;
  int l = (int)sqrtf(dx * dx + dy * dy);
  if (!l) return;
  dx = dx * 4 / l;
  dy = dy * 4 / l;
  grLine(x1 + dx, y1 + dy, x2 - dx, y2 - dy);
  x1 = (x1 + x2) / 2;
  y1 = (y1 + y2) / 2;
  grLine(x1, y1, x1 + dy, y1 - dx);
}

void edLine(int x1, int y1, int x2, int y2) {
  tr(&x1, &y1);
  tr(&x2, &y2);
  int dx = x2 - x1;
  int dy = y2 - y1;
  int l = (int)sqrtf(dx * dx + dy * dy);
  if (!l) return;
  dx = dx * 4 / l;
  dy = dy * 4 / l;
  grLine(x1 + dx, y1 + dy, x2 - dx, y2 - dy);
}

void edOctagon(int x, int y, int a) {
  tr(&x, &y);
  grOctagon(x, y, a);
}

void edObject(int x, int y, int alpha, int r, int c) {
  int a;
  float xp, yp;

  tr(&x, &y);
  a = r * zoom / 12;
  grSetColor(c);
  grOctagon(x, y, a);
  a /= 2;
  grBar(x - a, y - a, x + a, y + a);
  xp = (float)r * zoom * cosf(0.7854 * (alpha & 7)) * 0.12;
  yp = (float)r * zoom * sinf(0.7854 * (alpha & 7)) * 0.12;
  grLine(x, y, x + xp, y + yp);
}

void edStBegin() {
  grPushViewPort();
  grSetViewPort(tv.x1 - 1, tv.y2 + 1, tv.x2 + 1, tv.y2 + 19);
  grBegin();
  grSetColor(2);
  grBar(0, 0, tv.x2 - tv.x1 + 2, 18);
  grSetColor(255);
  grRectangle(0, 0, tv.x2 - tv.x1 + 2, 18);
  grSetPos(2, 0);
}

void edStEnd() {
  grEnd();
  grPopViewPort();
}

void edScreen() {
  int x, y, vx, vy, ix, iy;
  int bo;
  int r, c;

  grSetViewPort(0, 0, 0, 0);
  grBegin();
  grSetColor(0);
  grClear();
  grSetColor(255);
  grRectangle(tv.x1 - 1, tv.y1 - 1, tv.x2 + 1, tv.y2 + 1);
  grSetPos(tv.x1 + 8, tv.y1 - 20);
  grprintf("mode: %s", mdnames[mode]);
  grSetViewPort(tv.x1, tv.y1, tv.x2, tv.y2);
  grSetColor(250);
  x = 0, y = 0;
  tr(&x, &y);
  grLine(x, 0, x, 1000);
  grLine(0, y, 1000, y);
  grSetColor(254);
  if (grid) {
    ix = 0, iy = 0;
    for (;;) {
      bo = 0;
      vx = ix;  vy = iy;
      tr(&vx, &vy);
      if (vx > 0) {
        ix -= gs;
        ++bo;
      }
      if (vy < tv.y2-tv.y1) {
        iy -= gs;
        ++bo;
      }
      if (!bo) break;
    }
    vy = 1;
    for (y = iy; vy > 0; y += gs) {
      vx = 0;
      for (x = ix; vx <= tv.x2-tv.x1; x += gs) {
        vx = x;  vy = y;
        tr(&vx, &vy);
        grPutPixel(vx, vy);
      }
    }
  }
  switch (mode) {
    case MD_BSP:
      bspShow();
      break;
    case MD_SECTOR:
      grSetColor(255);
      for (unsigned i = 0; i < vc.size(); ++i)
        edVertex(vc[i].x, vc[i].y);
      grSetColor(78);
      for (unsigned i = 0; i < lc.size(); ++i)
        if (!lc[i].sf && !lc[i].sb)
          edLine(vc[lc[i].a].x, vc[lc[i].a].y, vc[lc[i].b].x, vc[lc[i].b].y);
      {
        int i = 0;
        for (unsigned j = 0; j < lc.size(); ++j) {
          if (lc[j].sf > i) i = lc[j].sf;
          if (lc[j].sb > i) i = lc[j].sb;
        }
        for (; i; --i) edSelectSector(i, 0);
      }
      break;
    case MD_OBJECT:
      grSetColor(78);
      for (unsigned i = 0; i < lc.size(); ++i) {
        if (lc[i].sf && lc[i].sb) /* 2 sided */
          grSetColor(76);
        else
          grSetColor(255);
        edLine(vc[lc[i].a].x, vc[lc[i].a].y, vc[lc[i].b].x, vc[lc[i].b].y);
      }
      for (unsigned i = 0; i < oc.size(); ++i) {
        edGetObjectProperties(oc[i].what, &r, &c);
        edObject(oc[i].x, oc[i].y, oc[i].c, r, c);
      }
      break;
    default:
      grSetColor(255);
      for (unsigned i = 0; i < vc.size(); ++i)
        edVertex(vc[i].x, vc[i].y);
      for (unsigned i = 0; i < lc.size(); ++i) {
        if (lc[i].sf && lc[i].sb) /* 2 sided */
          grSetColor(76);
        else
          grSetColor(255);
        edVector(vc[lc[i].a].x, vc[lc[i].a].y, vc[lc[i].b].x, vc[lc[i].b].y);
      }
  }
  grSetViewPort(0, 0, 0, 0);
  grEnd();
  grSetViewPort(tv.x1, tv.y1, tv.x2, tv.y2);
  moving = 0;
  sv = NULL;
  sl = NULL;
  so = NULL;
  ss = 0;
  tmpline.a = tmpline.b = -1;
  omx = omy = -1;
}

void
edSave()
{
  try {
    stWrite("map.st");
  } catch (const std::exception& e) {
    printf("failed to save map file: %s\n", e.what());
  }
}

void
edLoad()
{
  try {
    stRead("map.st");
  } catch (const std::exception& e) {
    printf("failed to load map file: %s\n", e.what());
  }
}

static void (*edMouseButtonMode)(int mx, int my, int button) = edMouseButtonVertex;
static void (*edMouseMotionMode)(int mx, int my, int umx, int umy) = edMouseMotionVertex;
static void (*edKeyboardMode)(int key) = edKeyboardVertex;

static int
edSaveSector(FILE *fp, Sector* s)
{
  unsigned char buf[2 * 2 + 1 + 2 * 2 + 4], *p = buf;
  *(short *)p = s->f;
  *(short *)(p + 2) = s->c;
  p[4] = s->l;
  *(unsigned short *)(p + 5) = s->u;
  *(unsigned short *)(p + 7) = s->v;
  *(unsigned *)(p + 9) = s->t;
  return (fwrite(buf, 1, sizeof(buf), fp) == sizeof(buf)) ? 0 : -1;
}

void edBuildBSP() {
  edSave();
  bspInit();
  int s = 0;
  for (unsigned i = 0; i < lc.size(); ++i) {
    if (lc[i].sf > s) s = lc[i].sf;
    if (lc[i].sb > s) s = lc[i].sb;
  }
  for (int j = s; j; --j)
    for (Lines::iterator l(lc.begin()); l != lc.end(); ++l) {
      int in = sc[j].f < sc[j].c;
      if (l->sf == j)
        bspAddLine(j, vc[l->a].x, vc[l->a].y, vc[l->b].x, vc[l->b].y,
          l->u, l->v, l->flags, (in) ? l->tf : l->tb, l->du);
      if (l->sb == j)
        bspAddLine(j, vc[l->b].x, vc[l->b].y, vc[l->a].x, vc[l->a].y,
          l->u, l->v, l->flags, (in) ? l->tb : l->tf, l->du);
    }
  bspBuildTree();
  FILE *f = fopen("map.bsp", "wb");
  ++s;
  fwrite(&s, sizeof(int), 1, f);
  for (Sector* sp = &sc.front(); sp < &sc.front() + s; ++sp) edSaveSector(f, sp);
  bspSave(f);
  edSaveObjects(f);
  fclose(f);
}

void edApplyMode() {
  edScreen();
  switch (mode) {
    case MD_VERTEX:
      edMouseButtonMode = edMouseButtonVertex;
      edMouseMotionMode = edMouseMotionVertex;
      edKeyboardMode = edKeyboardVertex;
      break;
    case MD_LINE:
      edMouseButtonMode = edMouseButtonLine;
      edMouseMotionMode = edMouseMotionLine;
      edKeyboardMode = edKeyboardLine;
      break;
    case MD_SECTOR:
      edMouseButtonMode = edMouseButtonSector;
      edMouseMotionMode = edMouseMotionSector;
      edKeyboardMode = edKeyboardSector;
      break;
    case MD_OBJECT:
      edMouseButtonMode = edMouseButtonObject;
      edMouseMotionMode = edMouseMotionObject;
      edKeyboardMode = edKeyboardObject;
      break;
    default:
      edMouseButtonMode = NULL;
      edMouseMotionMode = NULL;
      edKeyboardMode = NULL;
      break;
  }
}

void edInit() {
  tv.x1 = 20;
  tv.y1 = 20;
  tv.x2 = 1000;
  tv.y2 = 680;

  sx = sy = 0;
  int x = (tv.x2 - tv.x1) / 2;
  int y = (tv.y2 - tv.y1) / 2;
  itr(&x, &y);
  sx = -x;
  sy = -y;

  edLoad();
  edApplyMode();
  edScreen();
}

void edDone() {
  edSave();
}

void edKeyboard(int key) {
  switch (key) {
    case SDLK_ESCAPE:
      postQuit();
      break;
    case SDLK_SPACE:
      if (mode == MD_BSP) bspDone();
      edBuildBSP();
      mode = MD_BSP;
      edApplyMode();
      break;
    case SDLK_TAB:
      if (mode == MD_BSP) bspDone();
	  mode = Mode(mode + 1);
      if (mode >= MD_LAST) mode = MD_FIRST;
      edApplyMode();
      break;
    case SDLK_LEFT:
      sx -= gs;
      edScreen();
      break;
    case SDLK_RIGHT:
      sx += gs;
      edScreen();
      break;
    case SDLK_DOWN:
      sy -= gs;
      edScreen();
      break;
    case SDLK_UP:
      sy += gs;
      edScreen();
      break;
    case SDLK_KP_PLUS:
    case SDLK_HOME:
      zoom += 1;
      edScreen();
      break;
    case SDLK_KP_MINUS:
    case SDLK_END:
      if (zoom > 1) {
        zoom -= 1;
        edScreen();
      }
      break;
    case SDLK_g:
      grid ^= 1;
      edScreen();
      break;
    case SDLK_INSERT:
      if (gs > 1) {
        gs >>= 1;
        edScreen();
      }
      break;
    case SDLK_DELETE:
      gs <<= 1;
      edScreen();
      break;
    default:
      if (edKeyboardMode != NULL) edKeyboardMode(key);
      break;
  }
}

void edMouseButton(int mx, int my, int button) {
  grAlignMouse(&mx, &my);
  itr(&mx, &my);
  SDLMod m = SDL_GetModState();
  snap = !(m & KMOD_CTRL);
  if (snap) {
    mx += (mx > 0) ? gs/2 : -gs/2;
    my += (my > 0) ? gs/2 : -gs/2;
    mx = (mx) / gs * gs;
    my = (my) / gs * gs;
  }
  if (edMouseButtonMode != NULL) edMouseButtonMode(mx, my, button);
}

void edMouseMotion(int mx, int my, int state) {
  (void)state;
  static int delta = 0;
  grAlignMouse(&mx, &my);
  if (mx < 0 || my < 0) return;
  itr(&mx, &my);
  if ((mx ^ my) == delta) return;
  delta = mx ^ my;
  int umx = mx, umy = my;
  SDLMod m = SDL_GetModState();
  snap = !(m & KMOD_CTRL);
  if (snap) {
    mx += (mx > 0) ? gs/2 : -gs/2;
    my += (my > 0) ? gs/2 : -gs/2;
    mx = (mx) / gs * gs;
    my = (my) / gs * gs;
  }

  if (!moving && mode != MD_OBJECT) {
    Vertex *min = 0;
    for (unsigned i = 0; i < vc.size(); ++i) {
      const int dx = vc[i].x - umx;
      const int dy = vc[i].y - umy;
      vc[i].md = dx * dx + dy * dy;
      if (min == NULL || vc[i].md < min->md) min = &vc.front() + i;
    }
    if ((min != NULL || sv != NULL) && min != sv) {
      grBegin();
      if (sv != NULL) {
        grSetColor(255);
        edVertex(sv->x, sv->y);
      }
      if (min != NULL) {
        grSetColor(253);
        edVertex(min->x, min->y);
      }
      grEnd();
      if (min != NULL && mode == MD_VERTEX) {
        edStBegin();
        grprintf("vertex #%d - x:%d y:%d", min - &vc.front(), min->x, min->y);
        edStEnd();
      }
    }
    sv = min;
  }
  if (edMouseMotionMode != NULL) edMouseMotionMode(mx, my, umx, umy);

  omx = mx;
  omy = my;
}
