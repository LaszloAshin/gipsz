#include <stdlib.h>
#include <time.h>
#include <SDL/SDL.h>
#include "main.h"
#include "gr.h"
#include "ed.h"

void postQuit() {
  SDL_Event ev;
  ev.type = SDL_QUIT;
  SDL_PushEvent(&ev);
}

static void Update() {
  SDL_Event ev;
  if (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_KEYDOWNMASK) > 0)
    edKeyboard(ev.key.keysym.sym);
  if (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_MOUSEBUTTONUPMASK | SDL_MOUSEBUTTONDOWNMASK) > 0) {
    static Uint8 btn = 0;
    btn ^= ev.button.button;
    edMouseButton(ev.button.x, ev.button.y, btn);
    
  }
  if (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_MOUSEMOTIONMASK) > 0)
    edMouseMotion(ev.motion.x, ev.motion.y, ev.motion.state);
}

int SDLCALL EventFilter(const SDL_Event *ev) {
  switch (ev->type) {
    case SDL_KEYDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEMOTION:
    case SDL_QUIT:
      return 1;
    default:
      return 0;
  }
}


int main() {
  srand(time(NULL));
  SDL_Event ev;
  SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
  SDL_WM_SetCaption("ed", "map editor");
  SDL_Surface *sface = SDL_SetVideoMode(1024, 700, 8, 0);
  if (sface == NULL) {
    SDL_Quit();
    return 1;
  }
  if (!grSetSurface(sface)) {
    return 2;
  }
  SDL_EnableKeyRepeat(250, 30);
  SDL_EnableUNICODE(1);
  SDL_SetEventFilter(EventFilter);
/*  grBegin();
  int i;
  for (i = 0; i < 256; ++i) {
    grSetColor(i);
    grSetPos((i / 48)*32, (i % 48)*16);
    grprintf("%03d", i);
  }
  grEnd();*/
  edInit();
  while (!SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_QUITMASK)) {
    SDL_PumpEvents();
    Update();
    SDL_Delay(5);
  }
  edDone();
  SDL_Quit();
  return 0;
}
