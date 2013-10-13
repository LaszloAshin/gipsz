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

static enum {
  MD_FIRST = 0,
  MD_VERTEX = 0,
  MD_LINE,
  MD_SECTOR,
  MD_OBJECT,
  MD_LAST,
  MD_BSP,
} mode = MD_VERTEX;

static char *mdnames[] = {
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
  int i, j;
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
      for (i = 0; i < vc.n; ++i)
        edVertex(vc.p[i].x, vc.p[i].y);
      grSetColor(78);
      for (i = 0; i < lc.n; ++i)
        if (!lc.p[i].sf && !lc.p[i].sb)
          edLine(vc.p[lc.p[i].a].x, vc.p[lc.p[i].a].y, vc.p[lc.p[i].b].x, vc.p[lc.p[i].b].y);
      i = 0;
      for (j = 0; j < lc.n; ++j) {
        if (lc.p[j].sf > i) i = lc.p[j].sf;
        if (lc.p[j].sb > i) i = lc.p[j].sb;
      }
      for (; i; --i) edSelectSector(i, 0);
      break;
    case MD_OBJECT:
      grSetColor(78);
      for (i = 0; i < lc.n; ++i) {
        if (lc.p[i].sf && lc.p[i].sb) /* 2 sided */
          grSetColor(76);
        else
          grSetColor(255);
        edLine(vc.p[lc.p[i].a].x, vc.p[lc.p[i].a].y, vc.p[lc.p[i].b].x, vc.p[lc.p[i].b].y);
      }
      for (i = 0; i < oc.n; ++i) {
        edGetObjectProperties(oc.p[i].what, &r, &c);
        edObject(oc.p[i].x, oc.p[i].y, oc.p[i].c, r, c);
      }
      break;
    default:
      grSetColor(255);
      for (i = 0; i < vc.n; ++i)
        edVertex(vc.p[i].x, vc.p[i].y);
      for (i = 0; i < lc.n; ++i) {
        if (lc.p[i].sf && lc.p[i].sb) /* 2 sided */
          grSetColor(76);
        else
          grSetColor(255);
        edVector(vc.p[lc.p[i].a].x, vc.p[lc.p[i].a].y, vc.p[lc.p[i].b].x, vc.p[lc.p[i].b].y);
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

void edSave() {
  int i;
  stOpen();
  for (i = 0; i < vc.n; ++i) stPutVertex(vc.p + i);
  for (i = 0; i < lc.n; ++i) stPutLine(lc.p + i);
  for (i = 1; i < sc.alloc; ++i) stPutSector(i, sc.p + i);
  for (i = 0; i < oc.n; ++i) stPutObject(oc.p + i);
  stWrite("map.st");
  stClose();
}

void edLoad() {
  int i;
  int n;
  vertex_t v;
  line_t l;
  sector_t s;
  object_t o;

  stRead("map.st");
  vc.n = lc.n = 0;
  oc.n = 0;
  for (i = 0; i < sc.alloc; ++i) {
    sc.p[i].c = sc.p[i].f = sc.p[i].l = 0;
  }
  while (stGetVertex(&v)) {
    edAddVertex(v.x, v.y);
  }
  while (stGetLine(&l)) {
    if (l.sb == l.sf) l.sb = 0;
    edAddLine(l.a, l.b, l.sf, l.sb, l.u, l.v, l.flags, l.tf, l.tb, l.du);
  }
  while (stGetSector(&n, &s)) {
    edAddSector(n, s.f, s.c, s.l, s.u, s.v, s.t);
  }
  while (stGetObject(&o)) {
    edAddObject(o.what, o.x, o.y, o.z, o.a, o.b, o.c);
  }
  stClose();
}

static void (*edMouseButtonMode)(int mx, int my, int button) = edMouseButtonVertex;
static void (*edMouseMotionMode)(int mx, int my, int umx, int umy) = edMouseMotionVertex;
static void (*edKeyboardMode)(int key) = edKeyboardVertex;

void edBuildBSP() {
  int s = 0, i, j;
  line_t *l;
  edSave();
  bspInit();
  for (i = 0; i < lc.n; ++i) {
    if (lc.p[i].sf > s) s = lc.p[i].sf;
    if (lc.p[i].sb > s) s = lc.p[i].sb;
  }
  for (j = s; j; --j)
    for (i = 0, l = lc.p; i < lc.n; ++i, ++l) {
      int in = sc.p[j].f < sc.p[j].c;
      if (l->sf == j)
        bspAddLine(j, vc.p[l->a].x, vc.p[l->a].y, vc.p[l->b].x, vc.p[l->b].y,
          l->u, l->v, l->flags, (in) ? l->tf : l->tb, l->du);
      if (l->sb == j)
        bspAddLine(j, vc.p[l->b].x, vc.p[l->b].y, vc.p[l->a].x, vc.p[l->a].y,
          l->u, l->v, l->flags, (in) ? l->tb : l->tf, l->du);
    }
  bspBuildTree();
  FILE *f = fopen("map.bsp", "wb");
  ++s;
  fwrite(&s, sizeof(int), 1, f);
  fwrite(sc.p, sizeof(sector_t), s, f);
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

  vc.alloc = 8;
  vc.p = (vertex_t *)malloc(vc.alloc * sizeof(vertex_t));
  vc.n = 0;

  lc.alloc = 8;
  lc.p = (line_t *)malloc(lc.alloc * sizeof(line_t));
  lc.n = 0;

  sc.alloc = 2;
  sc.p = (sector_t *)malloc(sc.alloc * sizeof(sector_t));
  int i;
  for (i = 0; i < sc.alloc; ++i) sc.p[i].f = sc.p[i].c = sc.p[i].l = 0;

  oc.alloc = 8;
  oc.p = (object_t *)malloc(oc.alloc * sizeof(object_t));
  oc.n = 0;

  edLoad();
  edApplyMode();
  edScreen();
}

void edDone() {
  edSave();
  free(oc.p);
  free(sc.p);
  free(lc.p);
  free(vc.p);
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
      if (++mode >= MD_LAST) mode = MD_FIRST;
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
      zoom += 1;
      edScreen();
      break;
    case SDLK_KP_MINUS:
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
    int i;
    vertex_t *min = NULL;
    for (i = 0; i < vc.n; ++i) {
      int dx = vc.p[i].x - umx;
      int dy = vc.p[i].y - umy;
      vc.p[i].md = dx * dx + dy * dy;
      if (min == NULL || vc.p[i].md < min->md) min = vc.p + i;
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
        grprintf("vertex #%d - x:%d y:%d", min - vc.p, min->x, min->y);
        edStEnd();
      }
    }
    sv = min;
  }
  if (edMouseMotionMode != NULL) edMouseMotionMode(mx, my, umx, umy);

  omx = mx;
  omy = my;
}
