#ifndef _GAME_H
#define _GAME_H 1

#include <SDL/SDL.h>

void gDraw();
void gUpdate();
void gEnable();
void gDisable();
int gIsEnabled();
void gInit();
void gDone();
int gKey(SDL_KeyboardEvent *ev);

#endif /* _GAME_H */
