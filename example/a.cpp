#include "a.h"
#include <iostream>

__attribute__((visibility("default"))) void a::print(int i, std::string s) {
  std::cout << "Library A: " << i << " " << s << std::endl;
}
