#ifndef _MAIN_H
#define _MAIN_H 1

#include <SDL/SDL.h>

void postQuit();
void quit(int code);

int scrGetWidth();
int scrGetHeight();

void mouseHide();
void mouseShow();
void mouseCenter(int bo);

unsigned getFPS();

#endif /* _MAIN_H */
