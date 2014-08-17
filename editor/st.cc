/* vim: set ts=2 sw=8 tw=0 et :*/
#include "st.h"
#include "vertex.h"
#include "line.h"
#include "sector.h"
#include "object.h"

#include <lib/persistency.hh>

#include <cassert>
#include <stdexcept>
#include <fstream>
#include <iostream>

void
stWrite(const char* fname)
{
  std::ofstream f(fname);
  f.exceptions(std::ios::failbit | std::ios::badbit);
  if (!f) throw std::runtime_error("failed to open text map file for writing");
  f << "map v0.1" << std::endl;
  saveAllText("vertex", vc.begin(), vc.end(), f);
  saveAllText("line", lc.begin(), lc.end(), f);
  saveAllText("sector", sc.begin(), sc.end(), f);
  saveAllText("object", oc.begin(), oc.end(), f);
  std::cout << "text map written to " << fname << std::endl;
}

void
stRead(const char* fname)
{
  std::cerr << "reading text map from " << fname << std::endl;
  std::ifstream f(fname);
  f.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
  if (!f) throw std::runtime_error("failed to open text map file for reading");
  std::string magic, version;
  f >> magic >> version;
  if (magic != "map") throw std::runtime_error("invalid magic");
  if (version != "v0.1") throw std::runtime_error("invalid version");

  Vertexes vertexes;
  loadAllText("vertex", vertexes, f);

  Lines lines;
  loadAllText("line", lines, f);

  Sectors sectors;
  loadAllText("sector", sectors, f);

  Objects objects;
  loadAllText("object", objects, f);

  using std::swap;
  swap(vc, vertexes);
  swap(lc, lines);
  swap(sc, sectors);
  swap(oc, objects);
}
