#include "a.h"
#include <iostream>

__attribute__((visibility("default"))) void a::print(int i, std::string s) {
  std::cout << name << ": " << i << " " << s << std::endl;
}

const std::string a::name = "Library A";
