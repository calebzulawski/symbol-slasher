#include "a.h"
#include "b.h"

int main() {
  a::print(1, "a");
  b::print(2, "b");
  b::a::print(3, "a via b");
}
