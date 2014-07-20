/* vim: set ts=2 sw=8 tw=0 et :*/
#include "st.h"
#include "vertex.h"
#include "line.h"
#include "sector.h"
#include "object.h"

#include <cassert>
#include <stdexcept>
#include <fstream>

struct Header {
  unsigned nVerteces;
  unsigned nLines;
  unsigned nSectors;
  unsigned nObjects;
};

static Header
stReadHeader(std::istream& is)
{
  Header result;
  char buf[4 * 4], *p = buf;
  is.read(buf, sizeof(buf));
  result.nVerteces = *(unsigned *)p;
  result.nLines = *(unsigned *)(p + 4);
  result.nSectors = *(unsigned *)(p + 8);
  result.nObjects = *(unsigned *)(p + 12);
  return result;
}

static void
stWriteHeader(std::ostream& os)
{
  char buf[4 * 4], *p = buf;
  *(unsigned *)p = vc.size();
  *(unsigned *)(p + 4) = lc.size();
  *(unsigned *)(p + 8) = sc.size() ? (sc.size() - 1) : 0;
  *(unsigned *)(p + 12) = oc.size();
  os.write(buf, sizeof(buf));
}

static Vertex
stReadVertex(std::istream& is)
{
  Vertex result;
  char buf[2 * 2], *p = buf;
  is.read(buf, sizeof(buf));
  result.x = *(short *)p;
  result.y = *(short *)(p + 2);
  return result;
}

static void
stWriteVertex(std::ostream& os, const Vertex& v)
{
  char buf[2 * 2], *p = buf;
  *(short *)p = v.x;
  *(short *)(p + 2) = v.y;
  os.write(buf, sizeof(buf));
}

static Line
stReadLine(std::istream& is)
{
  Line result;
  char buf[6 * 2 + 1 + 2 * 4 + 2], *p = buf;
  is.read(buf, sizeof(buf));
  result.a = *(unsigned short *)p;
  result.b = *(unsigned short *)(p + 2);
  result.sf = *(unsigned short *)(p + 4);
  result.sb = *(unsigned short *)(p + 6);
  result.u = *(unsigned short *)(p + 8);
  result.v = *(unsigned short *)(p + 10);
  result.flags = buf[12];
  result.tf = *(unsigned *)(p + 13);
  result.tb = *(unsigned *)(p + 17);
  result.du = *(short *)(p + 21);
  if (result.sb == result.sf) result.sb = 0;
  return result;
}

static void
stWriteLine(std::ostream& os, const Line& l)
{
  char buf[6 * 2 + 1 + 2 * 4 + 2], *p = buf;
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
  os.write(buf, sizeof(buf));
}

static Sector
stReadSector(std::istream& is, unsigned& n)
{
  Sector result;
  char buf[3 * 2 + 1 + 2 * 2 + 4], *p = buf;
  is.read(buf, sizeof(buf));
  n = *(unsigned short *)p;
  result.f = *(short *)(p + 2);
  result.c = *(short *)(p + 4);
  result.l = buf[6];
  result.u = *(unsigned short *)(p + 7);
  result.v = *(unsigned short *)(p + 9);
  result.t = *(unsigned *)(p + 11);
  return result;
}

static void
stWriteSector(std::ostream& os, const Sector& s)
{
  char buf[3 * 2 + 1 + 2 * 2 + 4], *p = buf;
  unsigned n = &s - &sc.front();
  assert(n < sc.size());
  *(unsigned short *)p = n;
  *(short *)(p + 2) = s.f;
  *(short *)(p + 4) = s.c;
  buf[6] = s.l;
  *(unsigned short *)(p + 7) = s.u;
  *(unsigned short *)(p + 9) = s.v;
  *(unsigned *)(p + 11) = s.t;
  os.write(buf, sizeof(buf));
}

static Object
stReadObject(std::istream& is)
{
  Object result;
  char buf[5 * 2], *p = buf;
  is.read(buf, sizeof(buf));
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

static void
stWriteObject(std::ostream& os, const Object& o)
{
  char buf[5 * 2], *p = buf;
  *(short *)p = o.what;
  *(short *)(p + 2) = o.x;
  *(short *)(p + 4) = o.y;
  *(short *)(p + 6) = o.z;
  *(short *)(p + 8) = (o.a & 7) | ((o.b & 7) << 3) | ((o.c & 7) << 6);
  os.write(buf, sizeof(buf));
}

void
stWrite(const char* fname)
{
  std::ofstream f(fname, std::ios::binary);
  f.exceptions(std::ios::failbit | std::ios::badbit);
  if (!f) throw std::runtime_error("failed to open map file for writing");
  stWriteHeader(f);
  for (Vertexes::const_iterator i(vc.begin()); i != vc.end(); ++i) stWriteVertex(f, *i);
  for (Lines::const_iterator i(lc.begin()); i != lc.end(); ++i) stWriteLine(f, *i);
  {
    Sectors::const_iterator i(sc.begin());
    if (i != sc.end()) ++i;
    for (; i != sc.end(); ++i) stWriteSector(f, *i);
  }
  for (Objects::const_iterator i(oc.begin()); i != oc.end(); ++i) stWriteObject(f, *i);
  printf("map written to \"%s\"\n", fname);
}

void
stRead(const char* fname)
{
  std::ifstream f(fname, std::ios::binary);
  f.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
  if (!f) throw std::runtime_error("failed to open map file for reading");
  Header hdr(stReadHeader(f));
  puts("stRead");
  printf(
    "%u vertices, %u lines, %u sectors, %u objects\n",
    hdr.nVerteces,
    hdr.nLines,
    hdr.nSectors,
    hdr.nObjects
  );

  Vertexes vertexes;
  for (unsigned i = 0; i < hdr.nVerteces; ++i) {
    vertexes.push_back(stReadVertex(f));
    printf("vertex %u: x=%d y=%d\n", i, vertexes.back().x, vertexes.back().y);
  }

  Lines lines;
  for (unsigned i = 0; i < hdr.nLines; ++i) {
    lines.push_back(stReadLine(f));
    printf(
      "line %u: a=%u b=%u sf=%u sb=%u u=%u v=%u flg=%u tf=%u tb=%u du=%d\n",
      i,
      lines.back().a,
      lines.back().b,
      lines.back().sf,
      lines.back().sb,
      lines.back().u,
      lines.back().v,
      lines.back().flags,
      lines.back().tf,
      lines.back().tb,
      lines.back().du
    );
  }

  Sectors sectors;
  sectors.push_back(Sector());
  for (unsigned i = 0; i < hdr.nSectors; ++i) {
    unsigned n;
    sectors.push_back(stReadSector(f, n));
    if (n != i + 1) throw std::runtime_error("sector number");
    printf(
      "sector %u: s=%u f=%d c=%d l=%u u=%u v=%u t=%d\n",
      i,
      n,
      sectors.back().f,
      sectors.back().c,
      sectors.back().l,
      sectors.back().u,
      sectors.back().v,
      sectors.back().t
    );
  }

  Objects objects;
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

  using std::swap;
  swap(vc, vertexes);
  swap(lc, lines);
  swap(sc, sectors);
  swap(oc, objects);
}
