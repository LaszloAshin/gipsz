#include <stdlib.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "tga.h"
#include "main.h"
#include "menu.h"
#include "console.h"
#include "cmd.h"
#include "game.h"
#include "tex.h"
#include "mm.h"
#include "input.h"
#include "tx2.h"
#include "bsp.h"
#include "ogl.h"

static char version[] = "0.06 alpha";

static int cmd_version(int argc, char **argv) {
  (void)argc;
  (void)argv;
  cmsg(MLINFO, "version %s", version);
  return 0;
}

static unsigned fps;
static int drawfps = 1;
static int scrWidth = 1024;
static int scrHeight = 768;
static int scrbpp = 32;
static int scrFullScreen = 0;

int scrGetWidth() { return scrWidth; }
int scrGetHeight() { return scrHeight; }

void quit(int code) {
  SDL_Quit();
  exit(code);
}

void postQuit() {
  SDL_Event ev;
  ev.type = SDL_QUIT;
  SDL_PushEvent(&ev);
}

int cmd_quit(int argc, char **argv) {
  (void)argc;
  (void)argv;
  postQuit();
  return 0;
}

static int mVisible = 1;
static int mCenter = 0;
static GLuint ctex = 0;

void mouseInit() {
  if (ctex) return;
  tgaimage_t im;
  if (!tgaRead(&im, "data/gfx/cursor3.tga")) return;
  glGenTextures(1, &ctex);
  glBindTexture(GL_TEXTURE_2D, ctex);
  texImage(0, GL_ALPHA4, im.width, im.height, GL_ALPHA, im.data, im.bytes);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  tgaFree(&im);
  SDL_ShowCursor(0);
}

void mouseDone() {
  if (!ctex) return;
  glDeleteTextures(1, &ctex);
  ctex = 0;
}

void mouseHide() {
  if (!mVisible) return;
  mVisible = 0;
  if (!ctex) SDL_ShowCursor(0);
}

void mouseShow() {
  if (mVisible) return;
  mVisible = 1;
  if (!ctex) SDL_ShowCursor(1);
}

void mouseCenter(int bo) {
  mCenter = bo;
}

static void mouseDraw(double x, double y) {
  if (mCenter) x = y = 0.0;
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, ctex);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glBegin(GL_QUADS);
  glColor3d(1.0, 1.0, 1.0);
  glTexCoord2d(0.0, 0.0);
  glVertex2d(x + 0.04, y - 0.04 * scrWidth / scrHeight);
  glTexCoord2d(1.0, 0.0);
  glVertex2d(x + 0.04, y + 0.04 * scrWidth / scrHeight);
  glTexCoord2d(1.0, 1.0);
  glVertex2d(x - 0.04, y + 0.04 * scrWidth / scrHeight);
  glTexCoord2d(0.0, 1.0);
  glVertex2d(x - 0.04, y - 0.04 * scrWidth / scrHeight);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
}

static void Draw() {
  int x, y;
  double mx, my;
  SDL_GetMouseState(&x, &y);
  mx = 2.0 * x / scrWidth - 1.0;
  my = 1.0 - 2.0 * y / scrHeight;
  texUpdate();
  conBegin();
  if (gIsEnabled()) {
    gDraw(mx, my);
  } else {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  menuDraw(mx, my);
  conEnd();
  if (drawfps) dmsg(MLINFO, "%d fps", fps);
  if (mVisible && ctex) mouseDraw(mx, my);

  SDL_GL_SwapBuffers();
  GLenum gl_error = glGetError();
  if (gl_error != GL_NO_ERROR) cmsg(MLERR, "glGetError: %d", gl_error);
  char *sdl_error = SDL_GetError();
  if (*sdl_error) {
    cmsg(MLERR, "SDL_GetError: %s", sdl_error);
    SDL_ClearError();
  }
}

static void startgame() {
  if (bspIsLoaded()) {
    cmsg(MLWARN, "map is already loaded");
    return;
  }
  menuDisable();
  conExec("@devmap map");
}

static void InitVideo() {
  Uint32 vf = SDL_OPENGL;
  if (scrFullScreen) vf |= SDL_FULLSCREEN;
/*  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);*/
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  if (!SDL_SetVideoMode(scrWidth, scrHeight, scrbpp, vf)) {
    cmsg(MLERR, "SDL_SetVideoMode: %s", SDL_GetError());
    quit(1);
  }
  scrbpp = SDL_GetVideoSurface()->format->BitsPerPixel;
  SDL_WM_SetCaption("gipsz", "jakab");
  if (!scrFullScreen) SDL_WM_GrabInput(SDL_GRAB_ON);
  glViewport(0, 0, scrWidth, scrHeight);
}

static void PrepareMenu() {
  menuitem_t *p;
  menuAddItem(NULL, "Load map")->action = startgame;
  p = menuAddItem(NULL, "Submenu");
   menuAddItem(p, "Menupont #1");
   menuAddItem(p, "Menupont #2");
   menuAddItem(p, "Menupont #3");
  menuAddItem(NULL, "Menupont");
  menuAddItem(NULL, "Console")->action = conOpen;
  p = menuAddItem(NULL, "Quit");
   menuAddItem(p, "I am sure")->action = postQuit;
}

/* One frame takes at least this time */
#define FRAMETIME 1

static void storeGLInfo() {
  cmdAddString("GL_VENDOR", (char *)glGetString(GL_VENDOR), 0, 0);
  cmdAddString("GL_RENDERER", (char *)glGetString(GL_RENDERER), 0, 0);
  cmdAddString("GL_VERSION", (char *)glGetString(GL_VERSION), 0, 0);
//  cmdAddString("GL_EXTENSIONS", glGetString(GL_EXTENSIONS), 0, 0);
  cmdAddInteger("scrWidth", &scrWidth, 0);
  cmdAddInteger("scrHeight", &scrHeight, 0);
  cmdAddInteger("scrFullScreen", &scrFullScreen, 0);
  cmdAddInteger("scrbpp", &scrbpp, 0);
}

static Uint32 SDLCALL timer(Uint32 interval) {
  iGetActions(PT_HOLD, 0);
//  if (!gIsEnabled()) menuEnable();
  conUpdate();
  gUpdate();
  menuUpdate();
  return interval;
}

static int HandleEvent(SDL_Event *ev) {
  switch (ev->type) {
    case SDL_KEYDOWN:
      if (!conKey(&ev->key) && !menuKey(&ev->key) && !gKey(&ev->key))
        iGetActions(PT_PUSH, ev->key.keysym.sym);
      break;
    case SDL_KEYUP:
      iGetActions(PT_RELEASE, ev->key.keysym.sym);
      break;
    case SDL_MOUSEBUTTONDOWN:
      iGetActions(PT_PUSH, ev->button.button);
      break;
    case SDL_MOUSEBUTTONUP:
      if (!menuMouse(&ev->button))
        iGetActions(PT_RELEASE, ev->button.button);
      break;
    case SDL_QUIT: return 0;
  }
  return !0;
}

static void Init() {
  if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0) {
    cmsg(MLERR, "SDL_Init: %s", SDL_GetError());
    exit(1);
  }
  SDL_EnableKeyRepeat(250, 30);
  SDL_EnableUNICODE(1);
  mmInit();
  cmdInit();
  cmdAddCommand("quit", cmd_quit);
  cmdAddCommand("version", cmd_version);
  cmdAddBool("drawfps", &drawfps, 1);
  mmInitCmd();
  PrepareMenu();
  menuInit();
  InitVideo();
  mouseInit();
  tx2Init();
  conInit();
  oglInitProcs();
  storeGLInfo();
  texInit();
  gInit();
  iInit();
  SDL_SetTimer(10, timer);
  conExec("@exec data/autoexec.cmd");
}

static void Run() {
  int t, quit;
  static Uint32 ft[1200/FRAMETIME];

  t = SDL_GetTicks();
  for (unsigned i = 0; i < sizeof(ft) / sizeof(ft[0]); ++i) {
    ft[i] = t;
  }
  t = 0;
  quit = 0;
  do {
    t += FRAMETIME;
    if (t > 0) {
      Uint32 dt = SDL_GetTicks();
      Draw();
      for (unsigned i = (sizeof(ft) / sizeof(ft[0])) - 1; i; --i) {
        ft[i] = ft[i-1];
      }
      ft[0] = SDL_GetTicks();
      dt = ft[0] - dt;
      for (fps = 1; fps < (sizeof(ft) / sizeof(ft[0])); ++fps) {
        if (ft[fps] < ft[0] - 1000) break;
      }
      t -= dt;
      if (t > 0) {
        SDL_Delay(t);
        t = 0;
      }
    }
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) if (!HandleEvent(&ev)) ++quit;
  } while (!quit);
}

static void Done() {
  iDone();
  gDone();
  texDone();
  conDone();
  tx2Done();
  mouseDone();
//  printf("GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));
  menuDone();
  cmdFree();
  mmDone();
  SDL_Quit();
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  Init();
  Run();
  Done();
  return 0;
}
