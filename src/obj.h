#ifndef _OBJ_H
#define _OBJ_H 1

#include <stdio.h>

void objUpdate();
void *objGetForName(int name);
void *objAdd(int name, int modelname, float x, float y, float z);
void objDrawObjects();
int objInit();
void objFlush();
void objDone();
int objLoad(FILE *fp);

#endif /* _OBJ_H */
