#ifndef _ST2_H
#define _ST2_H 1

#include "vertex.h"
#include "line.h"
#include "sector.h"
#include "object.h"

int st2Open();
int st2Write(const char *fname);
int st2Read(const char *fname);
void st2Close();

int st2PutVertex(vertex_t *v);
int st2GetVertex(vertex_t *v);
int st2PutLine(line_t *l);
int st2GetLine(line_t *l);
int st2PutSector(int n, sector_t *s);
int st2GetSector(int *n, sector_t *s);

#endif /* _ST2_H */
