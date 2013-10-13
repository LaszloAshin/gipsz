#ifndef _MM_H
#define _MM_H 1

#include <stdio.h>

void *mmAlloc(int size);
void mmFree(void *p);
void mmInit();
void mmDone();
void mmInitCmd();

#endif /* _MM_H */

