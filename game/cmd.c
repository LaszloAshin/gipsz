#include <stdio.h>
#include <string.h>
#include "console.h"
#include "cmd.h"
#include "mm.h"

static const cvlistitem_t cvboollist[] = {
  { 0, "false" },
  { 0, "off" },
  { 0, "disabled" },
  { 0, "no" },
  { 1, "true" },
  { 1, "on" },
  { 1, "enabled" },
  { 1, "yes" },
  { 0, NULL }
};

typedef struct cmd_s {
  const char *name;
  cmdfunc_t func;
  struct cmd_s *next;
} cmd_t;

typedef struct cvar_s {
  const char *name;
  cvartype_t type;
  int size;
  void *addr;
  int rw;
  cvarreadfunc_t readfunc;
  cvarwritefunc_t writefunc;
  const cvlistitem_t *list;
  struct cvar_s *next;
} cvar_t;

static cmd_t *firstcmd = NULL, **lastcmd = &firstcmd;
static cvar_t *firstvar = NULL, **lastvar = &firstvar;

cmdfunc_t cmdGetFunc(const char *name) {
  cmd_t *p;
  for (p = firstcmd; p != NULL; p = p->next)
    if (!strcmp(name, p->name))
      return p->func;
  return NULL;
}

int cmdAddCommand(const char *name, cmdfunc_t func) {
  cmd_t *p;
  for (p = firstcmd; p != NULL; p = p->next)
    if (!strcmp(name, p->name)) break;
  if (p != NULL) {
    p->func = func;
  } else {
    p = (cmd_t *)mmAlloc(sizeof(cmd_t));
    if (p == NULL) return 0;
    p->name = name;
    p->func = func;
    p->next = NULL;
    *lastcmd = p;
    lastcmd = &p->next;
  }
  return !0;
}

static cvar_t *cmdAddVariable(const char *name, cvartype_t type, int size, void *addr, int rw) {
  cvar_t *p = (cvar_t *)mmAlloc(sizeof(cvar_t));
  if (p == NULL) return p;
  p->name = name;
  p->type = type;
  p->size = size;
  p->addr = addr;
  p->rw = rw;
  p->readfunc = NULL;
  p->writefunc = NULL;
  p->list = NULL;
  p->next = NULL;
  *lastvar = p;
  lastvar = &p->next;
  return p;
}

int cmdAddInteger(const char *name, int *var, int rw) {
  return cmdAddVariable(name, CVT_INT, sizeof(int), var, rw) != NULL;
}

int cmdAddFloat(const char *name, float *var, int rw) {
  return cmdAddVariable(name, CVT_FLOAT, sizeof(float), var, rw) != NULL;
}

int cmdAddDouble(const char *name, double *var, int rw) {
  return cmdAddVariable(name, CVT_DOUBLE, sizeof(double), var, rw) != NULL;
}

int cmdAddString(const char *name, char *var, int rw, int size) {
  return cmdAddVariable(name, CVT_STRING, size, var, rw) != NULL;
}

int cmdAddList(const char *name, int *var, const cvlistitem_t *list, int rw) {
  cvar_t *p = cmdAddVariable(name, CVT_LIST, 0, var, rw);
  if (p == NULL) return 0;
  p->list = list;
  return !0;
}

int cmdAddBool(const char *name, int *var, int rw) {
  return cmdAddList(name, var, cvboollist, rw);
}

int cmdSetAccessFuncs(const char *name, cvarreadfunc_t rf, cvarwritefunc_t wf) {
  cvar_t *p;
  for (p = firstvar; p != NULL; p = p->next)
    if (!strcmp(name, p->name)) {
      p->readfunc = rf;
      p->writefunc = wf;
      return !0;
    }
  return 0;
}

void cmdFree() {
  while (firstcmd != NULL) {
    cmd_t *p = firstcmd;
    firstcmd = p->next;
    mmFree(p);
  }
  lastcmd = &firstcmd;
  while (firstvar != NULL) {
    cvar_t *p = firstvar;
    firstvar = p->next;
    mmFree(p);
  }
  lastvar = &firstvar;
}

static int cmd_cmdlist(int argc, char **argv) {
  (void)argc;
  (void)argv;
  cmd_t *p;
  int i = 0;
  for (p = firstcmd; p != NULL; p = p->next, ++i)
    cmsg(MLINFO, " %08x %s", p->func, p->name);
  cmsg(MLINFO, "%d command(s)", i);
  return 0;
}

static void cmdPrintVar(cvar_t *p) {
  if (p->readfunc != NULL) p->readfunc(p->addr);
  if (p->addr == NULL) return;
  switch (p->type) {
    case CVT_INT:
      if (p->rw)
        cmsg(MLINFO, " %s %d", p->name, *(int *)p->addr);
      else
        cmsg(MLINFO, " %s (%d)", p->name, *(int *)p->addr);
      break;
    case CVT_FLOAT:
      if (p->rw)
        cmsg(MLINFO, " %s %f", p->name, *(float *)p->addr);
      else
        cmsg(MLINFO, " %s (%f)", p->name, *(float *)p->addr);
      break;
    case CVT_DOUBLE:
      if (p->rw)
        cmsg(MLINFO, " %s %lf", p->name, *(double *)p->addr);
      else
        cmsg(MLINFO, " %s (%lf)", p->name, *(double *)p->addr);
      break;
    case CVT_STRING: {
      char *s = p->addr;
      if (s == NULL)
        cmsg(MLINFO, " %s NULL", p->name);
      else if (p->rw)
        cmsg(MLINFO, " %s \"%s\"", p->name, s);
      else
        cmsg(MLINFO, " %s (%s)", p->name, s);
      break;
    }
    case CVT_LIST: {
      const cvlistitem_t *l;
      for (l = p->list; l->name != NULL; ++l)
        if (l->val == *(unsigned *)p->addr) {
          if (p->rw)
            cmsg(MLINFO, " %s %s", p->name, l->name);
          else
            cmsg(MLINFO, " %s (%s)", p->name, l->name);
          break;
        }
      if (l->name == NULL)
        cmsg(MLERR, " %s INVALID", p->name);
      break;
    }
  }
}

static int cmd_varlist(int argc, char **argv) {
  (void)argc;
  (void)argv;
  cvar_t *p;
  int i = 0;
  for (p = firstvar; p != NULL; p = p->next, ++i)
    cmdPrintVar(p);
  cmsg(MLINFO, "%d variable(s)", i);
  return 0;
}

static int cmd_get(int argc, char **argv) {
  if (argc <= 1) {
    cmsg(MLERR, "usage: %s varname1 [varname2 [varname3 [...]]]", argv[0]);
    return !0;
  }
  int i;
  for (i = 1; i < argc; ++i) {
    cvar_t *p;
    for (p = firstvar; p != NULL && strcmp(argv[i], p->name); p = p->next);
    if (p != NULL)
      cmdPrintVar(p);
    else
      cmsg(MLERR, "variable %s not found", argv[i]);
  }
  return 0;
}

static int cmd_set(int argc, char **argv) {
  if (argc < 2) {
   help:
    cmsg(MLERR, "usage: %s varname value", argv[0]);
    return !0;
  }
  cvar_t *p;
  for (p = firstvar; p != NULL && strcmp(argv[1], p->name); p = p->next);
  if (p == NULL) {
    cmsg(MLERR, "variable %s not found", argv[1]);
    return !0;
  }
  if (!p->rw) {
    cmsg(MLERR, "variable %s is read only", p->name);
    return !0;
  }
  if (p->addr != NULL) {
    switch (p->type) {
      case CVT_INT:
        if (argc < 3) goto help;
        if (sscanf(argv[2], "%d", (int *)p->addr)) break;
        cmsg(MLERR, "unable to parse \"%s\" as integer", argv[2]);
        return !0;
      case CVT_FLOAT:
        if (argc < 3) goto help;
        if (sscanf(argv[2], "%f", (float *)p->addr)) break;
        cmsg(MLERR, "unable to parse \"%s\" as float", argv[2]);
        return !0;
      case CVT_DOUBLE:
        if (argc < 3) goto help;
        if (sscanf(argv[2], "%lf", (double *)p->addr)) break;
        cmsg(MLERR, "unable to parse \"%s\" as double", argv[2]);
        return !0;
      case CVT_STRING: {
        char *d = p->addr, *e = d + p->size - 1;
        int i;
        for (i = 2; i < argc && d < e; ++i) {
          char *s = argv[i];
          for (; *s && d < e; ++s) *d++ = *s;
          if (d < e && argc > i+1) *d++ = ' ';
        }
        *d = '\0';
        break;
      }
      case CVT_LIST:
        if (argc < 3) goto help;
        const cvlistitem_t *l;
        for (l = p->list; l->name != NULL; ++l)
          if (!strcmp(argv[2], l->name)) {
            *(int *)p->addr = l->val;
            break;
          }
        if (l->name == NULL) {
          cmsg(MLERR, "invalid value");
          return !0;
        }
        break;
    }
  }
  if (p->writefunc != NULL) p->writefunc(p->addr);
  return 0;
}

static int cmd_toggle(int argc, char **argv) {
  if (argc < 1) {
    cmsg(MLERR, "usage: %s varname", argv[0]);
    return !0;
  }
  cvar_t *p;
  for (p = firstvar; p != NULL && strcmp(argv[1], p->name); p = p->next);
  if (p == NULL) {
    cmsg(MLERR, "variable %s not found", argv[1]);
    return !0;
  }
  if (!p->rw) {
    cmsg(MLERR, "variable %s is read only", p->name);
    return !0;
  }
  if (p->type != CVT_LIST || p->list != cvboollist) {
    cmsg(MLERR, "variable %s is not boolean", p->name);
    return !0;
  }
  int bo = 0;
  if (p->addr == NULL) {
    ++bo;
  } else {
    if (!strcmp(argv[0], "toggle")) {
      *(int *)p->addr = !*(int *)p->addr;
      ++bo;
    } else if (!strcmp(argv[0], "enable")) {
      if (!*(int *)p->addr) ++bo;
      *(int *)p->addr = !0;
    } else {
      if (*(int *)p->addr) ++bo;
      *(int *)p->addr = 0;
    }
  }
  if (bo && p->writefunc != NULL) p->writefunc(p->addr);
  return 0;
}

static int cmd_exec(int argc, char **argv) {
  char buf[256];
  if (argc < 2) {
    cmsg(MLERR, "nothing to execute");
    return !0;
  }
  FILE *f = fopen(argv[1], "rt");
  if (f == NULL) {
    cmsg(MLERR, "unable to open file %s for reading", argv[1]);
    return !0;
  }
  for (;;) {
    char *s = fgets(buf, 256, f);
    if (s != buf) break;
    s = s + strlen(s) - 1;
    if (*s == '\n') *s = '\0';
    s = strstr(buf, "//");
    if (s != NULL) *s = '\0';
    for (s = buf; *s < 32 && *s; ++s);
    if (*s) conExec(buf);
  }
  fclose(f);
  return 0;
}

int cmd_null(int argc, char **argv) {
  (void)argc;
  (void)argv;
  return 0;
}

void cmdInit() {
  cmdAddCommand("cmdlist", cmd_cmdlist);
  cmdAddCommand("varlist", cmd_varlist);
  cmdAddCommand("get", cmd_get);
  cmdAddCommand("set", cmd_set);
  cmdAddCommand("toggle", cmd_toggle);
  cmdAddCommand("enable", cmd_toggle);
  cmdAddCommand("disable", cmd_toggle);
  cmdAddCommand("exec", cmd_exec);
}
