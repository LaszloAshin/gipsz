#ifndef _ST_H
#define _ST_H 1

#include "vertex.h"
#include "line.h"
#include "sector.h"
#include "object.h"

int stOpen();
int stWrite(const char *fname);
int stRead(const char *fname);
void stClose();

int stGetSector(int* n, Sector* s);

#endif /* _ST_H */
