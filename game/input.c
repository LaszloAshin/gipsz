#include <string.h>
#include <SDL/SDL.h>
#include "input.h"
#include "mm.h"
#include "cmd.h"
#include "console.h"

typedef struct {
  int key;
  const char *name;
} ikey_t;

static const ikey_t ikey[] = {
  { SDL_BUTTON_LEFT, "mouse_left" },
  { SDL_BUTTON_MIDDLE, "mouse_mid" },
  { SDL_BUTTON_RIGHT, "mouse_right" },
  { SDL_BUTTON_WHEELUP, "mouse_whup" },
  { SDL_BUTTON_WHEELDOWN, "mouse_whdn" },
  { SDLK_BACKSPACE, "backspace" },
  { SDLK_TAB, "tab" },
  { SDLK_CLEAR, "clear" },
  { SDLK_RETURN, "enter" },
  { SDLK_PAUSE, "pause" },
  { SDLK_ESCAPE, "esc" },
  { SDLK_SPACE, "space" },
  { SDLK_EXCLAIM, "exclaim" },
  { SDLK_QUOTEDBL, "dblquote" },
  { SDLK_HASH, "hash" },
  { SDLK_DOLLAR, "dollar" },
  { SDLK_AMPERSAND, "ampersand" },
  { SDLK_QUOTE, "quote" },
  { SDLK_LEFTPAREN, "leftparen" },
  { SDLK_RIGHTPAREN, "rightparen" },
  { SDLK_ASTERISK, "asterisk" },
  { SDLK_PLUS, "plus" },
  { SDLK_COMMA, "comma" },
  { SDLK_MINUS, "minus" },
  { SDLK_PERIOD, "period" },
  { SDLK_SLASH, "slash" },
  { SDLK_0, "0" },
  { SDLK_1, "1" },
  { SDLK_2, "2" },
  { SDLK_3, "3" },
  { SDLK_4, "4" },
  { SDLK_5, "5" },
  { SDLK_6, "6" },
  { SDLK_7, "7" },
  { SDLK_8, "8" },
  { SDLK_9, "9" },
  { SDLK_COLON, "colon" },
  { SDLK_SEMICOLON, "semicolon" },
  { SDLK_LESS, "less" },
  { SDLK_EQUALS, "eq" },
  { SDLK_GREATER, "greater" },
  { SDLK_QUESTION, "question" },
  { SDLK_AT, "at" },
  { SDLK_LEFTBRACKET, "leftbracket" },
  { SDLK_BACKSLASH, "backslash" },
  { SDLK_RIGHTBRACKET, "rightbracket" },
  { SDLK_CARET, "caret" },
  { SDLK_UNDERSCORE, "underscore" },
  { SDLK_BACKQUOTE, "tilde" },
  { SDLK_a, "a" },
  { SDLK_b, "b" },
  { SDLK_c, "c" },
  { SDLK_d, "d" },
  { SDLK_e, "e" },
  { SDLK_f, "f" },
  { SDLK_g, "g" },
  { SDLK_h, "h" },
  { SDLK_i, "i" },
  { SDLK_j, "j" },
  { SDLK_k, "k" },
  { SDLK_l, "l" },
  { SDLK_m, "m" },
  { SDLK_n, "n" },
  { SDLK_o, "o" },
  { SDLK_p, "p" },
  { SDLK_q, "q" },
  { SDLK_r, "r" },
  { SDLK_s, "s" },
  { SDLK_t, "t" },
  { SDLK_u, "u" },
  { SDLK_v, "v" },
  { SDLK_w, "w" },
  { SDLK_x, "x" },
  { SDLK_y, "y" },
  { SDLK_z, "z" },
  { SDLK_DELETE, "del" },
  { SDLK_KP0, "kp0" },
  { SDLK_KP1, "kp1" },
  { SDLK_KP2, "kp2" },
  { SDLK_KP3, "kp3" },
  { SDLK_KP4, "kp4" },
  { SDLK_KP5, "kp5" },
  { SDLK_KP6, "kp6" },
  { SDLK_KP7, "kp7" },
  { SDLK_KP8, "kp8" },
  { SDLK_KP9, "kp9" },
  { SDLK_KP_PERIOD, "kp_period" },
  { SDLK_KP_DIVIDE, "kp_div" },
  { SDLK_KP_MULTIPLY, "kp_mult" },
  { SDLK_KP_MINUS, "kp_minus" },
  { SDLK_KP_PLUS, "kp_plus" },
  { SDLK_KP_ENTER, "kp_enter" },
  { SDLK_KP_EQUALS, "kp_eq" },
  { SDLK_UP, "up" },
  { SDLK_DOWN, "down" },
  { SDLK_RIGHT, "right" },
  { SDLK_LEFT, "left" },
  { SDLK_INSERT, "ins" },
  { SDLK_HOME, "home" },
  { SDLK_END, "end" },
  { SDLK_PAGEUP, "pgup" },
  { SDLK_PAGEDOWN, "pgdn" },
  { SDLK_F1, "f1" },
  { SDLK_F2, "f2" },
  { SDLK_F3, "f3" },
  { SDLK_F4, "f4" },
  { SDLK_F5, "f5" },
  { SDLK_F6, "f6" },
  { SDLK_F7, "f7" },
  { SDLK_F8, "f8" },
  { SDLK_F9, "f9" },
  { SDLK_F10, "f10" },
  { SDLK_F11, "f11" },
  { SDLK_F12, "f12" },
  { SDLK_F13, "f13" },
  { SDLK_F14, "f14" },
  { SDLK_F15, "f15" },
  { SDLK_NUMLOCK, "numlock" },
  { SDLK_CAPSLOCK, "caps" },
  { SDLK_SCROLLOCK, "scroll" },
  { SDLK_RSHIFT, "rshift" },
  { SDLK_LSHIFT, "lshift" },
  { SDLK_RCTRL, "rctrl" },
  { SDLK_LCTRL, "lctrl" },
  { SDLK_RALT, "ralt" },
  { SDLK_LALT, "lalt" },
  { SDLK_RMETA, "rmeta" },
  { SDLK_LMETA, "lmeta" },
  { SDLK_LSUPER, "lwin" },
  { SDLK_RSUPER, "rwin" },
  { SDLK_MODE, "altgr" },
  { SDLK_COMPOSE, "compose" },
  { SDLK_HELP, "help" },
  { SDLK_PRINT, "print" },
  { SDLK_SYSREQ, "sysrq" },
  { SDLK_BREAK, "break" },
  { SDLK_MENU, "menu" },
  { SDLK_POWER, "power" },
  { SDLK_EURO, "quro" },
  { SDLK_UNDO, "undo" },
};

typedef struct {
  int keyidx;
  ipushtype_t push;
  char *action;
} assoc_t;

static struct {
  assoc_t *p;
  unsigned alloc, n;
} ac;

static void iRemoveBinding(int keyidx) {
  for (unsigned i = 0; i < ac.n; ++i)
    if (ac.p[i].keyidx == keyidx) {
      mmFree(ac.p[i].action);
      ac.p[i] = ac.p[--ac.n];
      --i;
    }
}

static int cmd_unbind(int argc, char **argv) {
  if (argc < 2) {
    cmsg(MLERR, "usage: %s <key>", argv[0]);
    return !0;
  }
  int keyidx = -1;
  for (unsigned i = 0; i < sizeof(ikey) / sizeof(ikey_t); ++i)
    if (!strcmp(argv[1], ikey[i].name)) {
      keyidx = i;
      break;
    }
  if (keyidx < 0) {
    cmsg(MLERR, "no such key (%s)", argv[1]);
    return !0;
  }
  iRemoveBinding(keyidx);
  return 0;
}

static void iAddBinding(int keyidx, ipushtype_t push, char *action) {
  int na;

  for (unsigned i = 0; i < ac.n; ++i)
    if (ac.p[i].keyidx == keyidx && !strcmp(ac.p[i].action, action))
      return;
  if (ac.n == ac.alloc) {
    na = ac.alloc * 2;
    assoc_t *p = (assoc_t *)mmAlloc(na * sizeof(assoc_t));
    if (p == NULL) return;
    for (unsigned i = 0; i < ac.n; ++i) p[i] = ac.p[i];
    mmFree(ac.p);
    ac.p = p;
    ac.alloc = na;
  }
  ac.p[ac.n].keyidx = keyidx;
  ac.p[ac.n].action = (char *)mmAlloc(strlen(action) + 1);
  if (ac.p[ac.n].action == NULL) return;
  strcpy(ac.p[ac.n].action, action);
  ac.p[ac.n].push = push;
  ++ac.n;
}

static int cmd_bind(int argc, char **argv) {
  if (argc < 3) {
    cmsg(MLERR, "usage: %s <key> <action> [push|release]", argv[0]);
    return !0;
  }
  int keyidx = -1;
  for (unsigned i = 0; i < sizeof(ikey) / sizeof(ikey_t); ++i)
    if (!strcmp(argv[1], ikey[i].name)) {
      keyidx = i;
      break;
    }
  if (keyidx < 0) {
    cmsg(MLERR, "no such key (%s)", argv[1]);
    return !0;
  }
  ipushtype_t push = PT_HOLD;
  if (argc == 4) {
    if (!strcmp(argv[3], "push"))
      push = PT_PUSH;
    else if (!strcmp(argv[3], "release"))
      push = PT_RELEASE;
  }
  iRemoveBinding(keyidx);
  iAddBinding(keyidx, push, argv[2]);
  return 0;
}

void iFlushBindings() {
  for (unsigned i = 0; i < ac.n; ++i)
    mmFree(ac.p[i].action);
  ac.n = 0;
}

static int cmd_unbindall(int argc, char **argv) {
  (void)argc;
  (void)argv;
  iFlushBindings();
  return 0;
}

static Uint8 *ks = NULL;

void iGetActions(ipushtype_t push, int pkey) {
  int i;
  Uint8 ms = SDL_GetMouseState(NULL, NULL);
  assoc_t *a = ac.p;
  for (i = ac.n; i; --i, ++a) {
    int key = ikey[a->keyidx].key;
    if (key <= SDL_BUTTON_WHEELDOWN) {
      switch (a->push) {
        case PT_HOLD:
          if (ms & SDL_BUTTON(key)) conExec(a->action);
          break;
        case PT_PUSH:
          if (push == PT_PUSH && (SDL_BUTTON(pkey) & SDL_BUTTON(key)))
            conExec(a->action);
          break;
        case PT_RELEASE:
          if (push == PT_RELEASE && (SDL_BUTTON(pkey) & SDL_BUTTON(key)))
            conExec(a->action);
          break;
      }
    } else {
      switch (a->push) {
        case PT_HOLD:
          if (ks[key]) conExec(a->action);
          break;
        case PT_PUSH:
          if (push == PT_PUSH && pkey == key) conExec(a->action);
          break;
        case PT_RELEASE:
          if (push == PT_RELEASE && pkey == key) conExec(a->action);
          break;
      }
    }
  }
}

static int cmd_bindlist(int argc, char **argv) {
  (void)argc;
  (void)argv;
  for (unsigned i = 0; i < ac.n; ++i) {
    char *s = NULL;
    switch (ac.p[i].push) {
      case PT_HOLD:
        s = "hold";
        break;
      case PT_PUSH:
        s = "push";
        break;
      case PT_RELEASE:
        s = "release";
        break;
    }
    cmsg(MLINFO, " %s %s means \"%s\"", s, ikey[ac.p[i].keyidx].name, ac.p[i].action);
  }
  cmsg(MLINFO, "%d binding(s)", ac.n);
  return 0;
}

int iInit() {
  ac.alloc = 8;
  ac.p = (assoc_t *)mmAlloc(ac.alloc * sizeof(assoc_t));
  if (ac.p == NULL) return 0;
  ac.n = 0;
  ks = SDL_GetKeyState(NULL);
  cmdAddCommand("bind", cmd_bind);
  cmdAddCommand("unbind", cmd_unbind);
  cmdAddCommand("unbindall", cmd_unbindall);
  cmdAddCommand("bindlist", cmd_bindlist);
  return !0;
}

void iDone() {
  for (unsigned i = 0; i < ac.n; ++i) mmFree(ac.p[i].action);
  if (ac.p != NULL) {
    mmFree(ac.p);
    ac.p = NULL;
  }
  ac.n = ac.alloc = 0;
}
