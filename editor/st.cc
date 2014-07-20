/* vim: set ts=2 sw=8 tw=0 et :*/
#include "st.h"
#include "vertex.h"
#include "line.h"
#include "sector.h"
#include "object.h"

#include <cassert>
#include <stdexcept>
#include <fstream>
#include <iostream>

class Header {
public:
  Header(size_t vertexCount, size_t lineCount, size_t sectorCount, size_t objectCount)
  : vertexCount_(vertexCount)
  , lineCount_(lineCount)
  , sectorCount_(sectorCount)
  , objectCount_(objectCount)
  {}

  static Header load(std::istream& is);
  void save(std::ostream& os) const;
  void print(std::ostream& os) const;

  size_t vertexCount() const { return vertexCount_; }
  size_t lineCount() const { return lineCount_; }
  size_t sectorCount() const { return sectorCount_; }
  size_t objectCount() const { return objectCount_; }

private:
  size_t vertexCount_;
  size_t lineCount_;
  size_t sectorCount_;
  size_t objectCount_;
};

Header
Header::load(std::istream& is)
{
  char buf[4 * 4], *p = buf;
  is.read(buf, sizeof(buf));
  const size_t vertexCount = *(unsigned *)p;
  const size_t lineCount = *(unsigned *)(p + 4);
  const size_t sectorCount = *(unsigned *)(p + 8);
  const size_t objectCount = *(unsigned *)(p + 12);
  return Header(vertexCount, lineCount, sectorCount, objectCount);
}

void
Header::save(std::ostream& os)
const
{
  char buf[4 * 4], *p = buf;
  *(unsigned *)p = vertexCount();
  *(unsigned *)(p + 4) = lineCount();
  *(unsigned *)(p + 8) = sectorCount();
  *(unsigned *)(p + 12) = objectCount();
  os.write(buf, sizeof(buf));
}

void
Header::print(std::ostream& os)
const
{
  os << vertexCount_ << " verteces";
  os << ", " << lineCount_ << " lines";
  os << ", " << sectorCount_ << " sectors";
  os << ", " << objectCount_ << " objects";
  os << std::endl;
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
  Header(vc.size(), lc.size(), sc.size() ? (sc.size() - 1) : 0, oc.size()).save(f);
  for (Vertexes::const_iterator i(vc.begin()); i != vc.end(); ++i) i->save(f);
  for (Lines::const_iterator i(lc.begin()); i != lc.end(); ++i) i->save(f);
  {
    Sectors::const_iterator i(sc.begin());
    if (i != sc.end()) ++i;
    for (; i != sc.end(); ++i) i->save(f, &*i - &sc.front());
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
  const Header hdr(Header::load(f));
  puts("stRead");
  hdr.print(std::cout);

  Vertexes vertexes;
  for (unsigned i = 0; i < hdr.vertexCount(); ++i) {
    vertexes.push_back(Vertex::load(f));
    vertexes.back().print(std::cout, i);
  }

  Lines lines;
  for (unsigned i = 0; i < hdr.lineCount(); ++i) {
    lines.push_back(Line::load(f));
    lines.back().print(std::cout, i);
  }

  Sectors sectors;
  sectors.push_back(Sector());
  for (unsigned i = 0; i < hdr.sectorCount(); ++i) {
    unsigned n;
    sectors.push_back(Sector::load(f, n));
    if (n != i + 1) throw std::runtime_error("sector number");
    sectors.back().print(std::cout, i);
  }

  Objects objects;
  for (unsigned i = 0; i < hdr.objectCount(); ++i) {
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
