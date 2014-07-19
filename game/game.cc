#include <SDL/SDL_opengl.h>
#include "main.h"
#include "game.h"
#include "console.h"
#include "bsp.h"
#include "player.h"
#include "render.h"
#include "menu.h"
#include "obj.h"
#include "model.h"

static int enabled = 0;
int gIsEnabled() { return enabled; }

void gDraw() {
  if (!enabled) return;
  rBuildFrame();
}

void gUpdate() {
  if (!enabled) return;
  if (!menuIsEnabled()) {
    plUpdate();
    mouseCenter(1);
  }
  objUpdate();
}

int gKey(SDL_KeyboardEvent *ev) {
  if (!enabled) return 0;
  switch (ev->keysym.sym) {
    case SDLK_ESCAPE:
      menuEnable();
      break;
    default: return 0;
  }
  return !0;
}

void gEnable() {
  if (enabled) return;
  if (!bspIsLoaded()) {
    cmsg(MLERR, "gEnable: bsp is not ready");
    return;
  }
  ++enabled;
  int i;
  SDL_GetRelativeMouseState(&i, &i);
}

void gDisable() {
  if (!enabled) return;
  enabled = 0;
}

void gInit() {
  bspInit();
  plInit();
  rInit();
  modelInit();
  objInit();
/*
  objAdd(1, 1, 544, 544, 128);
  objAdd(2, 1, 544, 960, 128);
  objAdd(3, 1, 832, 1312, 128);
  objAdd(4, 1, 0, 1312, 128);
  objAdd(5, 2, 0, 256, 8);
  objAdd(6, 3, 0, 256, 40);
  objAdd(7, 4, 0, 384, 40);
  objAdd(8, 5, 0, 450, 40);*/
}

void gDone() {
  objDone();
  modelDone();
  rDone();
  plDone();
  bspDone();
}
