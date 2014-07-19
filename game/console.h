#ifndef _CONSOLE_H
#define _CONSOLE_H 1

#include <SDL/SDL.h>

typedef enum {
  CON_INACTIVE,
  CON_OPENING,
  CON_ACTIVE,
  CON_CLOSING
} constate_t;

typedef enum { MLERR, MLWARN, MLINFO, MLHINT, MLDBG } msglev_t;

void conOpen();
constate_t conGetState();
void conSetSize(unsigned w, unsigned h);
void conInit();
void conDone();
void cmsg(msglev_t lev, const char *fmt, ...);
void dmsg(msglev_t lev, const char *fmt, ...);
void conExec(const char *cmd);
void conBegin();
void conEnd();
void conUpdate();
int conKey(SDL_KeyboardEvent *ev);

#endif
