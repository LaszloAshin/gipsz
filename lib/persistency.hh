#ifndef LIB_PERSISTENCY_HEADER
#define LIB_PERSISTENCY_HEADER

#include <istream>
#include <ostream>
#include <vector>
#include <iterator>
#include <stdexcept>

template <class ForwardIterator>
void
saveAllText(const char* name, ForwardIterator first, ForwardIterator last, std::ostream& os)
{
  os << name << "-count " << std::distance(first, last) << std::endl;
  for (size_t i = 0; first != last; ++first, ++i) first->saveText(os, i);
}

template <class T>
void
loadAllText(const char* name, std::vector<T>& v, std::istream& is)
{
  std::string nameCount;
  is >> nameCount;
  if (nameCount != std::string(name) + "-count") throw std::runtime_error("unexpected count");
  size_t count;
  is >> count;
  std::vector<T>().swap(v);
  v.reserve(count);
  for (size_t i = 0; i < count; ++i) {
//    std::cerr << "loading " << name << " #" << i << std::endl;
    v.push_back(T::loadText(is, i));
  }
}


#endif // LIB_PERSISTENCY_HEADER
