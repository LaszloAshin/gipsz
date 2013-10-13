#include <stdio.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "menu.h"
#include "main.h"
#include "mm.h"
#include "tx2.h"
#include "game.h"

static menuitem_t *first = NULL, *curFirst = NULL;
static menuitem_t *selected = NULL;
static float theta = 0.0;
static int enabled = 0;
int menuIsEnabled() { return enabled; }

menuitem_t **menuGetEnd(menuitem_t **p) {
  if (p == NULL) return NULL;
  for (; *p != NULL; p = &(*p)->next);
  return p;
}

menuitem_t *menuAddItem(menuitem_t *parent, const char *caption) {
  menuitem_t *p = (menuitem_t *)mmAlloc(sizeof(menuitem_t));
  if (p == NULL) return p;
  p->parent = parent;
  p->caption = caption;
  p->first = p->next = NULL;
  p->action = NULL;
  p->root = (parent == NULL) ? first : parent->root;
  menuitem_t **t = menuGetEnd((parent == NULL) ? &first : &parent->first);
  *t = p;
  return p;
}

void menuFree(menuitem_t *first) {
  while (first != NULL) {
    menuitem_t *p = first;
    first = p->next;
    if (p->first != NULL) menuFree(p->first);
    mmFree(p);
  }
}

int menuHeight(menuitem_t *p) {
  int h = 0;
  for (; p != NULL; p = p->next) h++;
  return h;
}

void menuBack(menuitem_t *mi) {
  if (mi == NULL || mi->parent == NULL) {
    if (gIsEnabled()) menuDisable();
    return;
  }
  curFirst = mi->root;
  selected = mi->parent;
}

void menuActivate(menuitem_t *mi) {
  if (mi == NULL) return;
  if (mi->first != NULL)
    curFirst = selected = mi->first;
  if (mi->action != NULL)
    mi->action();
}

int menuKey(SDL_KeyboardEvent *ev) {
  if (!enabled) return 0;
  switch (ev->keysym.sym) {
    case SDLK_TAB:
    case SDLK_DOWN:
      if (selected != NULL && selected->next != NULL)
        selected = selected->next;
      else
        selected = curFirst;
      break;
    case SDLK_UP:
      if (selected == curFirst) selected = NULL;
      menuitem_t *p;
      for (p = curFirst; p != NULL; p = p->next)
        if (p->next == selected) {
          selected = p;
          break;
        }
      break;
    case SDLK_RIGHT:
    case SDLK_RETURN:
      menuActivate(selected);
      break;
    case SDLK_LEFT:
    case SDLK_ESCAPE:
      menuBack(selected);
      break;
    default: return 0;
  }
  return !0;
}

int menuMouse(SDL_MouseButtonEvent *ev) {
  if (!enabled) return 0;
  switch (ev->button) {
    case 1:
      menuActivate(selected);
      break;
    case 3:
      menuBack(selected);
      break;
    default: return 0;
  }
  return !0;
}

void menuUpdate() {
  if (!enabled) return;
  mouseCenter(0);
  theta += 0.1;
}

void menuDraw(float mx, float my) {
  if (!enabled) return;
  static float omx = 0.0, omy = 0.0;
  int chg = (mx != omx) || (my != omy);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glRotatef(theta, 0.0, 0.0, 1.0);
  glBegin(GL_TRIANGLES);
  glColor3f(0.5, 0.0, 0.0); glVertex2f(0.0, 0.5774);
  glColor3f(0.0, 0.5, 0.0); glVertex2f(-0.5, -0.2887);
  glColor3f(0.0, 0.0, 0.5); glVertex2f(0.5, -0.2887);
  glEnd();
  glLoadIdentity();
//  txSetFilter(TXF_UPPERCASE);
  tx2SetFixed(0);
  tx2SetHeight(0.16);
  tx2SetWidth(0.08);
  double h = tx2GetHeight();
  double y = h * menuHeight(curFirst) * 0.5;
  menuitem_t *mi;
  glColor3d(1.0, 1.0, 1.0);
  double c = 0.25 + 0.25 * sin(theta * 0.7);
  for (mi = curFirst; mi != NULL; mi = mi->next) {
    y -= h;
    double w = tx2GetStrWidth(mi->caption);
    double x = -w / 2;
    if (chg && mx >= x && mx <= x + w && my >= y && my <= y + h) selected = mi;
    if (mi == selected) glColor3d(1.0, c, 0.0);
    tx2PutStr(x, y, mi->caption);
    if (mi == selected) glColor3d(1.0, 1.0, 1.0);
  }
  omx = mx; omy = my;
}

void menuInit() {
  curFirst = selected = first;
  enabled = 1;
}

void menuDone() {
  menuFree(first);
  first = NULL;
}

void menuEnable() {
  if (enabled) return;
  mouseCenter(0);
  ++enabled;
}

void menuDisable() {
  if (!enabled) return;
  int i;
  SDL_GetRelativeMouseState(&i, &i);
  enabled = 0;
}
