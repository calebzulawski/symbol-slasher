#ifndef SYMBOL_SLASHER_EXAMPLE_B_H_
#define SYMBOL_SLASHER_EXAMPLE_B_H_
#include <string>

namespace b {
void print(int, std::string);
extern const std::string name;
}

namespace b::a {
void print(int, std::string);
}

#endif // SYMBOL_SLASHER_EXAMPLE_B_H_
