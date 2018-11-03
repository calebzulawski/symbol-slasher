#include "b.h"
#include "a.h"
#include <iostream>

__attribute__((visibility("default"))) void b::print(int i, std::string s) {
  std::cout << "Library B: " << i << " " << s << std::endl;
}

__attribute__((visibility("default"))) void b::a::print(int i, std::string s) {
  ::b::print(i, s);
  ::a::print(i, s);
}
