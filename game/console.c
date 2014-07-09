#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "main.h"
#include "console.h"
#include "cmd.h"
#include "mm.h"
#include "tex.h"
#include "tx2.h"
#include "ogl.h"

static constate_t conState = CON_INACTIVE;
static double conPos = 0.0;
static double conSize = 1.0;

#define CONTIME 50.0

static double conSpeed;
static double conAcc;
static char *conBuf = NULL;
static unsigned conBufWidth, conBufHeight;
static unsigned conCurPos = 0;
static int curBlank = 0;
static int retval = 0;
static int echo = 1;
static msglev_t verbosity = MLDBG;
static const cvlistitem_t verblevs[] = {
  { MLINFO, "MLINFO" },
  { MLHINT, "MLHINT" },
  { MLDBG, "MLDBG" },
  { 0, NULL }
};
static double cofs = 0.0;

#define CPRINTFLEN 8191
#define EDITLEN 511
#define HISTLEN 16383
static char edit[EDITLEN + 1];
static char hist[HISTLEN + 1], *hipo = hist;

constate_t conGetState() { return conState; }

typedef struct recmsg_s {
  int time;
  char msg[120];
  struct recmsg_s *next;
} recmsg_t;
static recmsg_t *rmfirst = NULL, **rmlast = &rmfirst;

void conLineFeed() {
  if (conBuf == NULL) return;
  conBuf[conBufWidth] = '\0';
  char *s, *d;
  unsigned len = (conBufWidth+1) * conBufHeight;
  d = conBuf + len - 1;
  s = d - (conBufWidth+1);
  for (len -= conBufWidth+1; len; --len)
    *d-- = *s--;
  for (len = conBufWidth+1; len; --len)
    *d-- = '\0';
}

void conWrite(const char *p) {
  printf("%s\n", p);
  if (conBuf == NULL) return;
  unsigned cp = 0;
  for (; *p; ++p) {
    switch (*p) {
      case '\n':
        cp = 0;
        break;
      default:
        if (!cp || cp >= conBufWidth) {
          conLineFeed();
          cp = 0;
        }
        conBuf[cp++] = *p;
        break;
    }
  }
}

void cmsg(msglev_t lev, const char *fmt, ...) {
  if (lev && lev > verbosity) return;
  va_list va;
  static char buf[CPRINTFLEN+1];
  va_start(va, fmt);
  vsnprintf(buf, CPRINTFLEN, fmt, va);
  va_end(va);
  buf[CPRINTFLEN] = '\0';
  conWrite(buf);
  recmsg_t *rm = mmAlloc(sizeof(recmsg_t));
  if (rm == NULL) return;
  rm->time = SDL_GetTicks();
  strncpy(rm->msg, buf, 120);
  rm->msg[119] = '\0';
  rm->next = NULL;
  *rmlast = rm;
  rmlast = &rm->next;
}

void rmClear(int t) {
  recmsg_t *p;

  while (rmfirst != NULL) {
    if (t && rmfirst->time > t) return;
    p = rmfirst;
    rmfirst = p->next;
    mmFree(p);
  }
  rmlast = &rmfirst;
}

static double cury;

void dmsg(msglev_t lev, const char *fmt, ...) {
  if (lev && lev > verbosity) return;
  va_list va;
  static char buf[64];
  va_start(va, fmt);
  vsnprintf(buf, 63, fmt, va);
  va_end(va);
  buf[63] = '\0';
  double h = 32.0 / scrGetHeight();
  cury -= h;
  tx2SetFixed(1);
  glColor3d(1.0, 1.0, 1.0);
  tx2PutStr(1.0 - tx2GetStrWidth(buf) - tx2GetWidth(), cury, buf);
}

static void conClose() {
  conAcc = conPos * 2.0 / (CONTIME * CONTIME);
  conSpeed = -conAcc * CONTIME;
  conState = CON_CLOSING;
}

static void conPushHist(char *s) {
  hipo = hist;
  int l = strlen(s);
  if (!l++) return;
  if (!strcmp(s, hist)) return;
  char *p;
  for (p = hist + HISTLEN; p > hist + l - 1; --p) *p = p[-l];
  strcpy(hist, s);
  hist[HISTLEN - 1] = hist[HISTLEN] = '\0';
}

void conExec(const char *cmd) {
  if (cmd == NULL) return;
  static char buf[512];
  strncpy(buf, cmd, 512);
  buf[511] = '\0';
  char *p;
  for (p = buf; *p < 32 && *p; ++p);
  int n = 0;
  if (*p == '@') {
    ++n;
    ++p;
  }
  if (echo && !n) {
    conPushHist(buf);
    conWrite(buf);
  }
  int iter;
  --p;
  do {
    iter = 0;
    ++p;
    n = 1; /* n must be >0 here */
    int quot = 0;
    int esc = 0;
    int argc = 1;
    char *q = buf;
    for (; *p; ++p)
      switch (*p) {
        case '\\':
          if (esc) {
            *q++ = *p;
            n = 0;
          }
          esc ^= 1;
          break;
        case '"':
          if (esc) {
            *q++ = *p;
            n = 0;
            esc = 0;
          } else
            quot ^= 1;
          break;
        case ' ':
          if (!n++) {
            *q++ = *p;
            if (!esc && !quot) {
              ++argc;
              q[-1] = '\0';
            }
          }
          esc = 0;
          break;
        case ';':
          if (esc || quot) {
            *q++ = *p;
            n = 0;
          } else {
           *p-- = '\0';
           ++iter;
          }
          esc = 0;
          break;
        default:
          if (*p > 32) *q++ = *p;
          n = 0;
          esc = 0;
          break;
      }
    while (q > buf && q[-1] < 32) --q;
    *q = '\0';
    if (quot) {
      cmsg(MLERR, "parse error: open quotes");
      return;
    }
    if (esc) {
      cmsg(MLERR, "parse error: open escape");
      return;
    }
    char *argv[argc], *r;
    n = 0;
    argv[n++] = buf;
    for (r = buf; n < argc; ++r) if (!*r) argv[n++] = ++r;
    cmdfunc_t func = cmdGetFunc(*argv);
    if (func != NULL) {
      retval = func(argc, argv);
    } else {
      cmsg(MLERR, "%s: command not found", argv[0]);
      retval = -1;
    }
  } while (iter);
}

int conKey(SDL_KeyboardEvent *ev) {
  if (conState == CON_INACTIVE) return 0;
  char *p;
  unsigned ch;
  switch (ev->keysym.sym) {
    case SDLK_BACKQUOTE:
      conClose();
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      if (strlen(edit)) conExec(edit);
      memset(edit, 0, sizeof(edit));
      conCurPos = 0;
      break;
    case SDLK_BACKSPACE:
      if (!conCurPos) break;
      --conCurPos;
    case SDLK_DELETE:
      p = edit + conCurPos;
      for (; *p; ++p) *p = p[1];
      *p = '\0';
      break;
    case SDLK_LEFT:
      if (conCurPos) --conCurPos;
      break;
    case SDLK_HOME:
      conCurPos = 0;
      break;
    case SDLK_END:
      conCurPos = strlen(edit);
      break;
    case SDLK_RIGHT:
      if (conCurPos < strlen(edit)) ++conCurPos;
      break;
    case SDLK_UP:
      if (*hipo == '\0') break;
      strcpy(edit, hipo);
      hipo += strlen(hipo) + 1;
      conCurPos = strlen(edit);
      break;
    case SDLK_DOWN:
      if (hipo <= hist || hipo[-1] != '\0') break;
      memset(edit, 0, sizeof(edit));
      for (--hipo; hipo > hist && hipo[-1] != '\0'; --hipo);
      if (hipo > hist && hipo[-1] == '\0') {
        char *p;
        for (p = hipo - 1; p > hist && p[-1] != '\0'; --p);
        strcpy(edit, p);
      }
      conCurPos = strlen(edit);
      break;
    case SDLK_PAGEUP:
      conSize -= 0.1;
      if (conSize < 0.1) conSize = 0.1;
      conPos = conSize;
      break;
    case SDLK_PAGEDOWN:
      conSize += 0.1;
      if (conSize > 2.0) conSize = 2.0;
      conPos = conSize;
      break;
    default:
      ch = ev->keysym.unicode;
      if (ch >= 32 && ch < 127 && conCurPos < EDITLEN) {
        if (strlen(edit) > conCurPos) {
          unsigned p = strlen(edit);
          for (; p > conCurPos; --p)
            edit[p] = edit[p-1];
        }
        if (edit[++conCurPos] == '\0') edit[conCurPos] = '\0';
        edit[conCurPos - 1] = ch;
      }
      break;
  }
  return !0;
}

void conUpdate() {
  if (conState == CON_INACTIVE) return;
  switch (conState) {
    case CON_OPENING:
      conPos += conSpeed;
      conSpeed += conAcc;
      if (conPos >= conSize) {
        conPos = conSize;
        conState = CON_ACTIVE;
      }
      break;
    case CON_CLOSING:
      conPos += conSpeed;
      conSpeed += conAcc;
      if (conPos <= 0) {
        conPos = 0.0;
        conState = CON_INACTIVE;
      }
      break;
    default: break;
  }
  ++curBlank;
  cofs += 0.0005;
}

static void conDrawRecent() {
  double h = 32.0 / scrGetHeight(), y = 1.0 - h - conPos;
  double x = 16.0 / scrGetWidth() - 1.0;
  recmsg_t *p;
  tx2SetFixed(1);
  glColor3d(1.0, 1.0, 1.0);
  for (p = rmfirst; p != NULL; p = p->next) {
    tx2PutStr(x, y, p->msg);
/*    char buf[20];
    sprintf(buf, "%d", p->time);
    tx2PutStr(0.0, y, buf);*/
    if (y < -1.0) break;
    y -= h;
  }
}

void conBegin() {
  rmClear((int)SDL_GetTicks() - 4000);
  cury = 1.0 - conPos;
}

void conEnd() {
  if (conState == CON_INACTIVE) {
    conDrawRecent();
    return;
  }
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  if (gl.ActiveTextureARB != NULL) {
    gl.ActiveTextureARB(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    texLoadTexture(1, 0);
    texSelectTexture(1);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(cofs * 0.2, cofs * 0.2, 0.0);
    glScalef(0.2 * sinf(cofs * 5.0) + 1.0, 0.2 * cosf(cofs * 3.0) + 1.0, 0.0);
    gl.ActiveTextureARB(GL_TEXTURE0);
  }

  glEnable(GL_TEXTURE_2D);
  texLoadTexture(11, 0);
  texSelectTexture(11);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glTranslatef(cofs + sinf(cofs) * 0.2, cofs, 0.0);

  glBegin(GL_QUADS);
  glColor3f(0.0, 1.0, 1.0);
  float h = 1.0 + conSize - conPos;
  glTexCoord2f(4.0, -2.0 * conSize);
  if (gl.MultiTexCoord2fARB != NULL)
    gl.MultiTexCoord2fARB(GL_TEXTURE1, 2.0, -conSize);
  glVertex2f(1.0, h);
  glTexCoord2f(0.0, -2.0 * conSize);
  if (gl.MultiTexCoord2fARB != NULL)
    gl.MultiTexCoord2fARB(GL_TEXTURE1, 0.0, -conSize);
  glVertex2f(-1.0, h);
  h -= conSize;
  glTexCoord2f(0.0, 0.0);
  if (gl.MultiTexCoord2fARB != NULL)
    gl.MultiTexCoord2fARB(GL_TEXTURE1, 0.0, 0.0);
  glVertex2f(-1.0, h);
  glTexCoord2f(4.0, 0.0);
  if (gl.MultiTexCoord2fARB != NULL)
    gl.MultiTexCoord2fARB(GL_TEXTURE1, 2.0, 0.0);
  glVertex2f(1.0, h);
  glEnd();

  glDisable(GL_TEXTURE_2D);
  if (gl.ActiveTextureARB != NULL) {
    gl.ActiveTextureARB(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    gl.ActiveTextureARB(GL_TEXTURE0);
  }

  if (conBuf == NULL) return;
  tx2SetFixed(1);
  glColor3f(1.0, 1.0, 1.0);
  float he = 32.0 / scrGetHeight();
  edit[EDITLEN] = '\0';
  int p = strlen(edit) / conBufWidth * conBufWidth;
  char buf[EDITLEN+1];
  memcpy(buf, edit, EDITLEN+1);
  for (; p >= 0; p -= conBufWidth) {
    tx2PutStr(-1.0, h, buf+p);
    buf[p] = '\0';
    h += he;
  }
  if (conState != CON_CLOSING && (curBlank >> 5) & 1) {
    float x = (float)(conCurPos % conBufWidth) * 2.0 / conBufWidth - 1.0;
    float y = h - he * (1 + conCurPos / conBufWidth);
    tx2PutStr(x, y, "_");
  }
  char *bp = conBuf;
  for (; h < 2.0; h += he, bp += conBufWidth+1) {
    if (bp >= conBuf + (conBufWidth+1) * conBufHeight) break;
    tx2PutStr(-1.0, h, bp);
  }
}

void conOpen() {
  if (conState != CON_INACTIVE) return;
  conAcc = (conPos - conSize) * 2.0 / (CONTIME * CONTIME);
  conSpeed = -conAcc * CONTIME;
  conState = CON_OPENING;
}


void conSetSize(unsigned w, unsigned h) {
  conBufWidth = conBufHeight = 0;
  if (conBuf != NULL) {
    mmFree(conBuf);
    conBuf = NULL;
  }
  if (!w || !h) return;
  conBuf = (char *)mmAlloc((w+1)*h);
  if (conBuf == NULL) return;
  memset(conBuf, 0, (w+1)*h);
  conBufWidth = w;
  conBufHeight = h;
}

static void conToggle(void *addr) {
  (void)addr;
  switch (conState) {
    case CON_INACTIVE:
      conOpen();
      break;
    case CON_ACTIVE:
      conClose();
      break;
    default: break;
  }
}

void conInit() {
  conSetSize(scrGetWidth() / 8, 100);
  memset(edit, 0, sizeof(edit));
  memset(hist, 0, sizeof(hist));
  hipo = hist;
  cmsg(MLINFO, "Console enters");
  cmdAddInteger("retval", &retval, 0);
  cmdAddBool("console", NULL, 1);
  cmdSetAccessFuncs("console", NULL, conToggle);
  cmdAddBool("echo", &echo, !0);
  cmdAddList("verbosity", (int *)&verbosity, verblevs, 1);
}

void conDone() {
  cmsg(MLINFO, "Console quits");
  conSetSize(0, 0);
  rmClear(0);
}
