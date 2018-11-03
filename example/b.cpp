#include "b.h"
#include "a.h"
#include <iostream>

void b::print(int i, std::string s) {
  std::cout << "Library B: " << i << " " << s << std::endl;
}

void b::a::print(int i, std::string s) {
  ::b::print(i, s);
  ::a::print(i, s);
}
