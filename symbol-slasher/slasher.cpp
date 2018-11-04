/* slasher.cpp
Copyright (C) 2018 Caleb Zulawski

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <LIEF/ELF/Parser.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unordered_map>

struct Symbol_map {
  Symbol_map(const char *store_path, bool read_only)
      : store(store_path, read_only
                              ? std::ios::in
                              : std::ios::in | std::ios::out | std::ios::app) {
    // Check that file actually opened
    if (!store.is_open() || !store.good())
      throw std::logic_error("failed to open hash store");

    // Parse any existing hashes, then reset the stream
    uint64_t hash;
    std::string name;
    while (store >> hash >> name)
      symbol_map[name] = hash;

    store.clear();
    store.seekg(0, std::ios::beg);

    next_hash_to_write = symbol_map.size();
  };

  ~Symbol_map() {
    // Dump only new hashes
    for (const auto &symbol : symbol_map) {
      if (symbol.second >= next_hash_to_write)
        store << symbol.second << " " << symbol.first << std::endl;
    }
  }

  void hash(std::string path) {
    auto object = LIEF::ELF::Parser::parse(path);
    if (!object)
      throw std::logic_error("could not open object file");
    auto symbols = object->dynamic_symbols();
    for (auto &symbol : symbols) {
      if (symbol.value() != 0 && symbol_map.count(symbol.name()) == 0)
        symbol_map[symbol.name()] = symbol_map.size();
    }
  }

  void slash(std::string in_path, std::string out_path) {
    auto object = LIEF::ELF::Parser::parse(in_path);
    if (!object)
      throw std::logic_error("could not open object file");
    auto symbols = object->dynamic_symbols();
    for (auto &symbol : symbols) {
      if (symbol_map.count(symbol.name()))
        symbol.name("sym_slash_" + std::to_string(symbol_map[symbol.name()]));
    }
    object->write(out_path);
  }

private:
  std::fstream store;
  std::unordered_map<std::string, uint64_t> symbol_map;
  uint64_t next_hash_to_write;
};

void usage(int code) {
  // clang-format off
  std::cout << "Usage: symbol-slasher hash|slash <arguments>" << std::endl;
  std::cout << std::endl;
  std::cout << "  symbol-slasher hash <store-path> <object-path...>" << std::endl;
  std::cout << "    Hashes the dynamic symbol names in each object provided by <object-path...>" << std::endl;
  std::cout << "    and updates the table stored in <store-path>.  If the hash store does not exist," << std::endl;
  std::cout << "    it is created." << std::endl;
  std::cout << std::endl;
  std::cout << "  symbol-slasher slash <store-path> <input-object-path> <output-object-path>"
            << std::endl;
  std::cout << "    Replaces all of the symbol names in <input-object-path> with the hashes in" << std::endl;
  std::cout << "    <store-path> and writes the modified object to <output-object-path>." << std::endl;
  // clang-format on
  std::exit(code);
}

int main(int argc, char **argv) try {
  if (argc < 2)
    usage(-1);

  std::string mode(argv[1]);
  if (mode == "hash") {
    if (argc < 4)
      usage(-1);
    Symbol_map map(argv[2], false);
    for (int i = 3; i < argc; i++)
      map.hash(argv[i]);
  } else if (mode == "slash") {
    if (argc != 5)
      usage(-1);
    Symbol_map map(argv[2], true);
    map.slash(argv[3], argv[4]);
  }
  return 0;
} catch (const std::exception &e) {
  std::cerr << "Error: " << e.what() << std::endl;
  return -1;
}
