/* vim: set ts=2 sw=8 tw=0 et :*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "st.h"
#include "vertex.h"
#include "line.h"
#include "sector.h"
#include "object.h"

#include <cassert>
#include <stdexcept>

struct Header {
  unsigned nVerteces;
  unsigned nLines;
  unsigned nSectors;
  unsigned nObjects;
};

typedef struct {
  unsigned short a;
  unsigned short b;
  unsigned short sf;
  unsigned short sb;
  unsigned short u;
  unsigned short v;
  unsigned char flags;
  unsigned int tf;
  unsigned int tb;
  short du;
} fline_t;

typedef struct {
  unsigned short s;
  short f;
  short c;
  unsigned char l;
  unsigned short u;
  unsigned short v;
  unsigned int t;
} fsector_t;

static struct {
  fline_t *p;
  unsigned n, alloc, r;
} flc;

static struct {
  fsector_t *p;
  unsigned n, alloc, r;
} fsc;

static int inited = 0;

void stClose() {
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

  inited = 0;
}

int stOpen() {
  if (inited) return 0;
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

  ++inited;
  return !0;
}

static Header
stReadHeader(FILE *fp)
{
  Header result;
  unsigned char buf[4 * 4], *p = buf;
  if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)) throw std::runtime_error("header");
  result.nVerteces = *(unsigned *)p;
  result.nLines = *(unsigned *)(p + 4);
  result.nSectors = *(unsigned *)(p + 8);
  result.nObjects = *(unsigned *)(p + 12);
  return result;
}

static int
stWriteHeader(FILE *fp)
{
  unsigned char buf[4 * 4], *p = buf;
  *(unsigned *)p = vc.size();
  *(unsigned *)(p + 4) = lc.size();
  *(unsigned *)(p + 8) = sc.size() ? (sc.size() - 1) : 0;
  *(unsigned *)(p + 12) = oc.size();
  return (fwrite(buf, 1, sizeof(buf), fp) == sizeof(buf)) ? 0 : -1;
}

static Vertex
stReadVertex(FILE *fp)
{
  Vertex result;
  unsigned char buf[2 * 2], *p = buf;
  if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)) throw std::runtime_error("vertex");
  result.x = *(short *)p;
  result.y = *(short *)(p + 2);
  return result;
}

static int
stWriteVertex(FILE *fp, const Vertex& v)
{
  unsigned char buf[2 * 2], *p = buf;
  *(short *)p = v.x;
  *(short *)(p + 2) = v.y;
  return (fwrite(buf, 1, sizeof(buf), fp) == sizeof(buf)) ? 0 : -1;
}

static int
stReadLine(FILE *fp, fline_t *l)
{
  unsigned char buf[6 * 2 + 1 + 2 * 4 + 2], *p = buf;
  if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)) return -1;
  l->a = *(unsigned short *)p;
  l->b = *(unsigned short *)(p + 2);
  l->sf = *(unsigned short *)(p + 4);
  l->sb = *(unsigned short *)(p + 6);
  l->u = *(unsigned short *)(p + 8);
  l->v = *(unsigned short *)(p + 10);
  l->flags = buf[12];
  l->tf = *(unsigned *)(p + 13);
  l->tb = *(unsigned *)(p + 17);
  l->du = *(short *)(p + 21);
  return 0;
}

static int
stWriteLine(FILE *fp, const Line& l)
{
  unsigned char buf[6 * 2 + 1 + 2 * 4 + 2], *p = buf;
  *(unsigned short *)p = l.a;
  *(unsigned short *)(p + 2) = l.b;
  *(unsigned short *)(p + 4) = l.sf;
  *(unsigned short *)(p + 6) = l.sb;
  *(unsigned short *)(p + 8) = l.u;
  *(unsigned short *)(p + 10) = l.v;
  buf[12] = l.flags;
  *(unsigned *)(p + 13) = l.tf;
  *(unsigned *)(p + 17) = l.tb;
  *(short *)(p + 21) = l.du;
  return (fwrite(buf, 1, sizeof(buf), fp) == sizeof(buf)) ? 0 : -1;
}

static int
stReadSector(FILE *fp, fsector_t *s)
{
  unsigned char buf[3 * 2 + 1 + 2 * 2 + 4], *p = buf;
  if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)) return -1;
  s->s = *(unsigned short *)p;
  s->f = *(short *)(p + 2);
  s->c = *(short *)(p + 4);
  s->l = buf[6];
  s->u = *(unsigned short *)(p + 7);
  s->v = *(unsigned short *)(p + 9);
  s->t = *(unsigned *)(p + 11);
  return 0;
}

static int
stWriteSector(FILE *fp, const Sector& s)
{
  unsigned char buf[3 * 2 + 1 + 2 * 2 + 4], *p = buf;
  unsigned n = &s - &sc.front();
  assert(n < sc.size());
  *(unsigned short *)p = n;
  *(short *)(p + 2) = s.f;
  *(short *)(p + 4) = s.c;
  buf[6] = s.l;
  *(unsigned short *)(p + 7) = s.u;
  *(unsigned short *)(p + 9) = s.v;
  *(unsigned *)(p + 11) = s.t;
  return (fwrite(buf, 1, sizeof(buf), fp) == sizeof(buf)) ? 0 : -1;
}

static Object
stReadObject(FILE *fp)
{
  Object result;
  unsigned char buf[5 * 2], *p = buf;
  if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)) throw std::runtime_error("object");
  result.what = ObjType::Type(*(short *)p);
  result.x = *(short *)(p + 2);
  result.y = *(short *)(p + 4);
  result.z = *(short *)(p + 6);
  const short rot = *(short *)(p + 8);
  result.a = rot & 7;
  result.b = (rot >> 3) & 7;
  result.c = (rot >> 6) & 7;
  return result;
}

static int
stWriteObject(FILE *fp, const Object& o)
{
  unsigned char buf[5 * 2], *p = buf;
  *(short *)p = o.what;
  *(short *)(p + 2) = o.x;
  *(short *)(p + 4) = o.y;
  *(short *)(p + 6) = o.z;
  *(short *)(p + 8) = (o.a & 7) | ((o.b & 7) << 3) | ((o.c & 7) << 6);
  return (fwrite(buf, 1, sizeof(buf), fp) == sizeof(buf)) ? 0 : -1;
}

int stWrite(const char *fname) {
  int ret = 0;
  if (!inited || fname == NULL) return ret;
  FILE *f = fopen(fname, "wb");
  if (f == NULL) return ret;
  if (stWriteHeader(f)) goto end;
  for (Vertexes::const_iterator i(vc.begin()); i != vc.end(); ++i) {
    if (stWriteVertex(f, *i)) goto end;
  }
  for (Lines::const_iterator i(lc.begin()); i != lc.end(); ++i) {
    if (stWriteLine(f, *i)) goto end;
  }
  {
    Sectors::const_iterator i(sc.begin());
    if (i != sc.end()) ++i;
    for (; i != sc.end(); ++i) {
      if (stWriteSector(f, *i)) goto end;
    }
  }
  for (Objects::const_iterator i(oc.begin()); i != oc.end(); ++i) {
    if (stWriteObject(f, *i)) goto end;
  }
  ++ret;
  printf("map written to \"%s\"\n", fname);
 end:
  fclose(f);
  return ret;
}

int stRead(const char *fname) {
  Vertexes vertexes;
  Objects objects;
  if (fname == NULL) return 0;
  FILE *f = fopen(fname, "rb");
  if (f == NULL) return 0;
  Header hdr(stReadHeader(f));
  puts("stRead");
  printf(
    "%u vertices, %u lines, %u sectors, %u objects\n",
    hdr.nVerteces,
    hdr.nLines,
    hdr.nSectors,
    hdr.nObjects
  );
  if (inited) stClose();

  flc.alloc = flc.n = hdr.nLines;
  flc.p = (fline_t *)malloc(flc.alloc * sizeof(fline_t));
  if (flc.p == NULL) goto end2;

  fsc.alloc = fsc.n = hdr.nSectors;
  fsc.p = (fsector_t *)malloc(fsc.alloc * sizeof(fsector_t));
  if (fsc.p == NULL) goto end2;

  for (unsigned i = 0; i < hdr.nVerteces; ++i) {
    vertexes.push_back(stReadVertex(f));
    printf("vertex %u: x=%d y=%d\n", i, vertexes.back().x, vertexes.back().y);
  }

  for (unsigned i = 0; i < flc.n; ++i) {
    if (stReadLine(f, flc.p + i)) goto end2;
    printf(
      "line %u: a=%u b=%u sf=%u sb=%u u=%u v=%u flg=%u tf=%u tb=%u du=%d\n",
      i,
      flc.p[i].a,
      flc.p[i].b,
      flc.p[i].sf,
      flc.p[i].sb,
      flc.p[i].u,
      flc.p[i].v,
      flc.p[i].flags,
      flc.p[i].tf,
      flc.p[i].tb,
      flc.p[i].du
    );
  }
  for (unsigned i = 0; i < fsc.n; ++i) {
    if (stReadSector(f, fsc.p + i)) goto end2;
    printf(
      "sector %u: s=%u f=%d c=%d l=%u u=%u v=%u t=%d\n",
      i,
      fsc.p[i].s,
      fsc.p[i].f,
      fsc.p[i].c,
      fsc.p[i].l,
      fsc.p[i].u,
      fsc.p[i].v,
      fsc.p[i].t
    );
  }

  for (unsigned i = 0; i < hdr.nObjects; ++i) {
    objects.push_back(stReadObject(f));
    printf(
      "object %u: t=%d x=%d y=%d z=%d a=%d b=%d c=%d\n",
      i,
      objects.back().what,
      objects.back().x,
      objects.back().y,
      objects.back().z,
      objects.back().a,
      objects.back().b,
      objects.back().c
    );
  }

  fclose(f);
  flc.r = fsc.r = 0;
  ++inited;
  using std::swap;
  swap(vc, vertexes);
  swap(oc, objects);
  return !0;
 end2:
  stClose();
  fclose(f);
  return 0;
}

int stGetLine(Line* l) {
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

int stGetSector(int *n, Sector* s) {
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
