#ifndef _OBJ_H
#define _OBJ_H 1

#include <istream>

struct Obj;

void objUpdate();
Obj *objGetForName(int name);
Obj *objAdd(int name, int modelname, float x, float y, float z);
void objDrawObjects();
int objInit();
void objFlush();
void objDone();
void objLoad(std::istream& is);

#endif /* _OBJ_H */
