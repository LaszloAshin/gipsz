#ifndef _INPUT_H
#define _INPUT_H 1

typedef enum { PT_HOLD, PT_PUSH, PT_RELEASE } ipushtype_t;

void iFlushBindings();
void iGetActions(ipushtype_t push, int key);
int iInit();
void iDone();

#endif /* _INPUT_H */
