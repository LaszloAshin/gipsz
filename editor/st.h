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

int stPutVertex(Vertex* v);
int stGetVertex(Vertex* v);
int stPutLine(line_t *l);
int stGetLine(line_t *l);
int stPutSector(int n, sector_t *s);
int stGetSector(int *n, sector_t *s);
int stPutObject(object_t *o);
int stGetObject(object_t *o);

#endif /* _ST_H */
