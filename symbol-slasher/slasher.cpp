/* slasher.cpp
This file is part of Symbol Slasher.

Symbol Slasher is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Symbol Slasher is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Symbol Slasher.  If not, see <https://www.gnu.org/licenses/>.
*/

extern "C" {
#include <bfd.h>
}

#include <iostream>
#include <unordered_map>
#include <vector>

struct Bfd_exception : std::exception {
  Bfd_exception() : msg(bfd_errmsg(bfd_get_error())) {}
  const char *what() const noexcept { return msg; }

private:
  const char *msg = nullptr;
};

struct Symbol_map {
  void hash(const char *path) {
    auto descriptor = bfd_openr(path, nullptr);
    if (descriptor == nullptr)
      throw Bfd_exception();

    if (!bfd_check_format(descriptor, bfd_object))
      throw Bfd_exception();

    auto symbols_size = bfd_get_dynamic_symtab_upper_bound(descriptor);
    if (symbols_size < 0)
      throw Bfd_exception();
    symbol_cache.resize(symbols_size);

    auto symbols = reinterpret_cast<bfd_symbol **>(symbol_cache.data());
    auto symbol_count = bfd_canonicalize_dynamic_symtab(descriptor, symbols);
    if (symbol_count < 0)
      throw Bfd_exception();

    for (int i = 0; i < symbol_count; i++) {
      if (symbols[i]->value != 0)
        symbol_hashes[symbols[i]->name] = symbol_hashes.size();
    }
  }

  void print() {
    for (auto symbol : symbol_hashes)
      std::cout << symbol.second << " " << symbol.first << std::endl;
  }

private:
  std::vector<uint8_t> symbol_cache;
  std::unordered_map<std::string, uint64_t> symbol_hashes;
};

int main(int argc, char **argv) try {
  bfd_init();
  if (argc < 2)
    return -1;

  Symbol_map map;
  for (int i = 1; i < argc; i++)
    map.hash(argv[i]);
  map.print();
  return 0;
} catch (const Bfd_exception &e) {
  std::cerr << "libbfd error: " << e.what() << std::endl;
  return -1;
} catch (const std::exception &e) {
  std::cerr << "exception: " << e.what() << std::endl;
  return -1;
}
