#ifndef _MENU_H
#define _MENU_H 1

#include "main.h"

typedef struct menuitem_s {
  struct menuitem_s *next, *parent, *first, *root;
  const char *caption;
  void (*action)(void);
} menuitem_t;

void menuInit();
void menuDone();
menuitem_t *menuAddItem(menuitem_t *parent, const char *caption);
void menuEnable();
void menuDisable();
int menuIsEnabled();

int menuKey(SDL_KeyboardEvent *ev);
int menuMouse(SDL_MouseButtonEvent *ev);
void menuUpdate();
void menuDraw(float mx, float my);

#endif
