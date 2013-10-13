#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "st.h"
#include "vertex.h"
#include "line.h"
#include "sector.h"
#include "object.h"

typedef struct {
  unsigned nVerteces;
  unsigned nLines;
  unsigned nSectors;
  unsigned nObjects;
} header_t;

typedef struct {
  short x : 16;
  short y : 16;
} __attribute__((packed)) fvertex_t;

typedef struct {
  unsigned short a : 16;
  unsigned short b : 16;
  unsigned short sf : 16;
  unsigned short sb : 16;
  unsigned short u : 16;
  unsigned short v : 16;
  unsigned char flags : 8;
  unsigned int tf : 32;
  unsigned int tb : 32;
  short du : 16;
} __attribute__((packed)) fline_t;

typedef struct {
  unsigned short s : 16;
  short f : 16;
  short c : 16;
  unsigned char l : 8;
  unsigned short u : 16;
  unsigned short v : 16;
  unsigned int t : 32;
} __attribute__((packed)) fsector_t;

typedef struct {
  short type : 16;
  short x : 16;
  short y : 16;
  short z : 16;
  short rot : 16;
} __attribute__((packed)) fobject_t;

static struct {
  fvertex_t *p;
  unsigned n, alloc, r;
} fvc;

static struct {
  fline_t *p;
  unsigned n, alloc, r;
} flc;

static struct {
  fsector_t *p;
  unsigned n, alloc, r;
} fsc;

static struct {
  fobject_t *p;
  unsigned n, alloc, r;
} foc;

static int inited = 0;

void stClose() {
  if (foc.p != NULL) {
    free(foc.p);
    foc.p = NULL;
  }
  foc.n = foc.alloc = 0;

  if (fsc.p != NULL) {
    free(fsc.p);
    fsc.p = NULL;
  }
  fsc.n = fsc.alloc = 0;

  if (flc.p != NULL) {
    free(flc.p);
    flc.p = NULL;
  }
  flc.n = flc.alloc = 0;

  if (fvc.p != NULL) {
    free(fvc.p);
    fvc.p = NULL;
  }
  fvc.n = fvc.alloc = 0;

  inited = 0;
}

int stOpen() {
  if (inited) return 0;
  fvc.alloc = 8;
  fvc.p = (fvertex_t *)malloc(fvc.alloc * sizeof(fvertex_t));
  if (fvc.p == NULL) {
    stClose();
    return 0;
  }
  fvc.n = 0;

  flc.alloc = 8;
  flc.p = (fline_t *)malloc(flc.alloc * sizeof(fline_t));
  if (flc.p == NULL) {
    stClose();
    return 0;
  }
  flc.n = 0;

  fsc.alloc = 8;
  fsc.p = (fsector_t *)malloc(fsc.alloc * sizeof(fsector_t));
  if (fsc.p == NULL) {
    stClose();
    return 0;
  }
  fsc.n = 0;

  foc.alloc = 8;
  foc.p = (fobject_t *)malloc(foc.alloc * sizeof(fobject_t));
  if (foc.p == NULL) {
    stClose();
    return 0;
  }
  foc.n = 0;

  ++inited;
  return !0;
}

int stWrite(const char *fname) {
  int ret = 0;
  if (!inited || fname == NULL) return ret;
  FILE *f = fopen(fname, "wb");
  if (f == NULL) return ret;
  header_t hdr;
  hdr.nVerteces = fvc.n;
  hdr.nLines = flc.n;
  hdr.nSectors = fsc.n;
  hdr.nObjects = foc.n;
  if (!fwrite(&hdr, sizeof(header_t), 1, f)) goto end;
  if (fwrite(fvc.p, sizeof(fvertex_t), fvc.n, f) != fvc.n) goto end;
  if (fwrite(flc.p, sizeof(fline_t), flc.n, f) != flc.n) goto end;
  if (fwrite(fsc.p, sizeof(fsector_t), fsc.n, f) != fsc.n) goto end;
  if (fwrite(foc.p, sizeof(fobject_t), foc.n, f) != foc.n) goto end;
  ++ret;
 end:
  fclose(f);
  return ret;
}

int stRead(const char *fname) {
  if (fname == NULL) return 0;
  FILE *f = fopen(fname, "rb");
  if (f == NULL) return 0;
  header_t hdr;
  if (!fread(&hdr, sizeof(header_t), 1, f)) goto end1;
  if (inited) stClose();

  fvc.alloc = fvc.n = hdr.nVerteces;
  fvc.p = (fvertex_t *)malloc(fvc.alloc * sizeof(fvertex_t));
  if (fvc.p == NULL) goto end2;

  flc.alloc = flc.n = hdr.nLines;
  flc.p = (fline_t *)malloc(flc.alloc * sizeof(fline_t));
  if (flc.p == NULL) goto end2;

  fsc.alloc = fsc.n = hdr.nSectors;
  fsc.p = (fsector_t *)malloc(fsc.alloc * sizeof(fsector_t));
  if (fsc.p == NULL) goto end2;

  foc.alloc = foc.n = hdr.nObjects;
  foc.p = (fobject_t *)malloc(foc.alloc * sizeof(fobject_t));
  if (foc.p == NULL) goto end2;

  if (fread(fvc.p, sizeof(fvertex_t), fvc.n, f) != fvc.n) goto end2;
  if (fread(flc.p, sizeof(fline_t), flc.n, f) != flc.n) goto end2;
/*  {
    memset(flc.p, 0, flc.n * sizeof(fline_t));
    int i;
    for (i = 0; i < flc.n; ++i) fread(flc.p + i, sizeof(fline_t)-8, 1, f);
  }*/
  if (fread(fsc.p, sizeof(fsector_t), fsc.n, f) != fsc.n) goto end2;
  if (fread(foc.p, sizeof(fobject_t), foc.n, f) != foc.n) goto end2;

  fclose(f);
  fvc.r = flc.r = fsc.r = foc.r = 0;
  ++inited;
  return !0;
 end2:
  stClose();
 end1:
  fclose(f);
  return 0;
}

int stPutVertex(vertex_t *v) {
  int i;
  int na;

  if (fvc.n == fvc.alloc) {
    na = fvc.alloc * 2;
    fvertex_t *p = (fvertex_t *)malloc(na * sizeof(fvertex_t));
    if (p == NULL) return -1;
    for (i = 0; i < fvc.n; ++i) p[i] = fvc.p[i];
    free(fvc.p);
    fvc.p = p;
    fvc.alloc = na;
  }
  fvc.p[fvc.n].x = v->x;
  fvc.p[fvc.n].y = v->y;
  return fvc.n++;
}

int stGetVertex(vertex_t *v) {
  if (fvc.r == fvc.n) {
    fvc.r = 0;
    return 0;
  }
  v->x = fvc.p[fvc.r].x;
  v->y = fvc.p[fvc.r].y;
  ++fvc.r;
  return !0;
}

int stPutLine(line_t *l) {
  int i;
  int na;

  if (flc.n == flc.alloc) {
    na = flc.alloc * 2;
    fline_t *p = (fline_t *)malloc(na * sizeof(fline_t));
    if (p == NULL) return -1;
    for (i = 0; i < flc.n; ++i) p[i] = flc.p[i];
    free(flc.p);
    flc.p = p;
    flc.alloc = na;
  }
  flc.p[flc.n].a = l->a;
  flc.p[flc.n].b = l->b;
  flc.p[flc.n].sf = l->sf;
  flc.p[flc.n].sb = l->sb;
  flc.p[flc.n].u = l->u;
  flc.p[flc.n].v = l->v;
  flc.p[flc.n].flags = l->flags;
  flc.p[flc.n].tf = l->tf;
  flc.p[flc.n].tb = l->tb;
  flc.p[flc.n].du = l->du;
  return flc.n++;
}

int stGetLine(line_t *l) {
  if (flc.r == flc.n) {
    flc.r = 0;
    return 0;
  }
  l->a = flc.p[flc.r].a;
  l->b = flc.p[flc.r].b;
  l->sf = flc.p[flc.r].sf;
  l->sb = flc.p[flc.r].sb;
  l->u = flc.p[flc.r].u;
  l->v = flc.p[flc.r].v;
  l->flags = flc.p[flc.r].flags;
  l->tf = flc.p[flc.r].tf;
  l->tb = flc.p[flc.r].tb;
  l->du = flc.p[flc.r].du;
  ++flc.r;
  return !0;
}

int stPutSector(int n, sector_t *s) {
  int i;
  int na;

  if (fsc.n == fsc.alloc) {
    na = fsc.alloc * 2;
    fsector_t *p = (fsector_t *)malloc(na * sizeof(fsector_t));
    if (p == NULL) return -1;
    for (i = 0; i < fsc.n; ++i) p[i] = fsc.p[i];
    free(fsc.p);
    fsc.p = p;
    fsc.alloc = na;
  }
  fsc.p[fsc.n].s = n;
  fsc.p[fsc.n].f = s->f;
  fsc.p[fsc.n].c = s->c;
  fsc.p[fsc.n].l = s->l;
  fsc.p[fsc.n].u = s->u;
  fsc.p[fsc.n].v = s->v;
  fsc.p[fsc.n].t = s->t;
  return fsc.n++;
}

int stGetSector(int *n, sector_t *s) {
  if (fsc.r == fsc.n) {
    fsc.r = 0;
    return 0;
  }
  *n = fsc.p[fsc.r].s;
  s->f = fsc.p[fsc.r].f;
  s->c = fsc.p[fsc.r].c;
  s->l = fsc.p[fsc.r].l;
  s->u = fsc.p[fsc.r].u;
  s->v = fsc.p[fsc.r].v;
  s->t = fsc.p[fsc.r].t;
  ++fsc.r;
  return !0;
}

int stPutObject(object_t *o) {
  int i;
  int na;

  if (foc.n == foc.alloc) {
    na = foc.alloc * 2;
    fobject_t *p = (fobject_t *)malloc(na * sizeof(fobject_t));
    if (p == NULL) return -1;
    for (i = 0; i < foc.n; ++i) p[i] = foc.p[i];
    free(foc.p);
    foc.p = p;
    foc.alloc = na;
  }
  foc.p[foc.n].type = o->what;
  foc.p[foc.n].x = o->x;
  foc.p[foc.n].y = o->y;
  foc.p[foc.n].z = o->z;
  foc.p[foc.n].rot = (o->a & 7) | ((o->b & 7) << 3) | ((o->c & 7) << 6);
  return foc.n++;
}

int stGetObject(object_t *o) {
  if (foc.r == foc.n) {
    foc.r = 0;
    return 0;
  }
  o->what = foc.p[foc.r].type;
  o->x = foc.p[foc.r].x;
  o->y = foc.p[foc.r].y;
  o->z = foc.p[foc.r].z;
  o->a = foc.p[foc.r].rot & 7;
  o->b = (foc.p[foc.r].rot >> 3) & 7;
  o->c = (foc.p[foc.r].rot >> 6) & 7;
  ++foc.r;
  return !0;
}
