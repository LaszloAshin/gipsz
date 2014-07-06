#ifndef _CON_CMD_H
#define _CON_CMD_H 1

typedef int (*cmdfunc_t)(int argc, char **argv);
typedef struct {
  unsigned val;
  const char *name;
} cvlistitem_t;
typedef void (*cvarreadfunc_t)(void *);
typedef void (*cvarwritefunc_t)(void *);
typedef enum {
  CVT_INT,
  CVT_FLOAT,
  CVT_DOUBLE,
  CVT_STRING,
  CVT_LIST,
} cvartype_t;

int cmdAddCommand(const char *name, cmdfunc_t func);
int cmdAddInteger(const char *name, int *var, int rw);
int cmdAddFloat(const char *name, float *var, int rw);
int cmdAddDouble(const char *name, double *var, int rw);
int cmdAddString(const char *name, char *var, int rw, int size);
int cmdAddList(const char *name, int *var, const cvlistitem_t *list, int rw);
int cmdAddBool(const char *name, int *var, int rw);
int cmdSetAccessFuncs(const char *name, cvarreadfunc_t rf, cvarwritefunc_t wf);
void cmdFree();
cmdfunc_t cmdGetFunc(const char *name);
void cmdInit();

int cmd_null(int argc, char **argv);

#endif /* _CON_CMD_H */
