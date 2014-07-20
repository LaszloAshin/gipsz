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

template <class ForwardIterator>
void
saveAll(ForwardIterator first, ForwardIterator last, std::ostream& os)
{
  for (; first != last; ++first) first->save(os);
}

void
stWrite(const char* fname)
{
  std::ofstream f(fname, std::ios::binary);
  f.exceptions(std::ios::failbit | std::ios::badbit);
  if (!f) throw std::runtime_error("failed to open map file for writing");
  Header(vc.size(), lc.size(), sc.size() ? (sc.size() - 1) : 0, oc.size()).save(f);
  saveAll(vc.begin(), vc.end(), f);
  saveAll(lc.begin(), lc.end(), f);
  {
    Sectors::const_iterator i(sc.begin());
    if (i != sc.end()) ++i;
    for (; i != sc.end(); ++i) i->save(f, &*i - &sc.front());
  }
  saveAll(oc.begin(), oc.end(), f);
  std::cout << "map written to " << fname << std::endl;
}

template <class T>
void
loadAll(std::vector<T>& v, size_t count, std::istream& is, std::ostream& os)
{
  for (size_t i = 0; i < count; ++i) {
    v.push_back(T::load(is));
    v.back().print(os, i);
  }
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
  loadAll(vertexes, hdr.vertexCount(), f, std::cout);

  Lines lines;
  loadAll(lines, hdr.lineCount(), f, std::cout);

  Sectors sectors;
  sectors.push_back(Sector());
  for (unsigned i = 0; i < hdr.sectorCount(); ++i) {
    unsigned n;
    sectors.push_back(Sector::load(f, n));
    if (n != i + 1) throw std::runtime_error("sector number");
    sectors.back().print(std::cout, i);
  }

  Objects objects;
  loadAll(objects, hdr.objectCount(), f, std::cout);

  using std::swap;
  swap(vc, vertexes);
  swap(lc, lines);
  swap(sc, sectors);
  swap(oc, objects);
}
