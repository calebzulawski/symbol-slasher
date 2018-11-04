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
#include <string>
#include <unordered_map>

constexpr auto prefix = "symslash";

struct Store_base {
  Store_base(bool read_only) : read_only(read_only) {}

  virtual void open(const char *store_path) {
    store.open(store_path, read_only
                               ? std::ios::in
                               : std::ios::in | std::ios::out | std::ios::app);

    // Check that file actually opened
    if (!store.is_open() || !store.good())
      throw std::logic_error("failed to open hash store");

    // Parse any existing hashes, then reset the stream
    uint64_t hash;
    std::string name;
    while (store >> hash >> name)
      insert(name, hash);

    store.clear();
    store.seekg(0, std::ios::beg);
  }

  virtual ~Store_base(){};

protected:
  std::fstream store;

  const bool read_only;

private:
  virtual void insert(std::string name, uint64_t hash){};
};

struct Forward_map : public Store_base {
  Forward_map(bool read_only) : Store_base(read_only) {}

  void open(const char *store_path) {
    Store_base::open(store_path);
    next_hash_to_write = symbol_map.size();
  }

  ~Forward_map() {
    // Dump only new hashes
    if (!read_only) {
      for (const auto &symbol : symbol_map) {
        if (symbol.second >= next_hash_to_write)
          store << symbol.second << " " << symbol.first << std::endl;
      }
    }
  }

  void insert(std::string name) {
    if (symbol_map.count(name) == 0)
      symbol_map[name] = symbol_map.size();
  }

  std::string hash(std::string name) {
    return symbol_map.count(name) == 0
               ? name
               : std::string(prefix) + std::to_string(symbol_map[name]);
  }

private:
  void insert(std::string name, uint64_t hash) override {
    symbol_map[name] = hash;
  }

  std::unordered_map<std::string, uint64_t> symbol_map;
  uint64_t next_hash_to_write;
};

struct Reverse_map : public Store_base {
  Reverse_map() : Store_base(false) {}

  std::string dehash(std::string name) {
    return symbol_map.count(name) == 0 ? name : symbol_map[name];
  }

private:
  void insert(std::string name, uint64_t hash) override {
    symbol_map[std::string(prefix) + std::to_string(hash)] = name;
  }

  std::unordered_map<std::string, std::string> symbol_map;
};

struct Inserter : public Forward_map {
  Inserter() : Forward_map(false) {}

  void operator()(std::string path) {
    auto object = LIEF::ELF::Parser::parse(path);
    if (!object)
      throw std::logic_error("could not open object file");
    auto symbols = object->dynamic_symbols();
    for (auto &symbol : symbols) {
      if (symbol.value() != 0)
        insert(symbol.name());
    }
  }
};

struct Hasher : public Forward_map {
  Hasher() : Forward_map(true) {}

  void operator()(std::string in_path, std::string out_path) {
    auto object = LIEF::ELF::Parser::parse(in_path);
    if (!object)
      throw std::logic_error("could not open object file");
    auto symbols = object->dynamic_symbols();
    for (auto &symbol : symbols)
      symbol.name(hash(symbol.name()));
    object->write(out_path);
  }
};

struct Dehasher : public Reverse_map {
  void operator()(std::string in_path, std::string out_path) {
    auto object = LIEF::ELF::Parser::parse(in_path);
    if (!object)
      throw std::logic_error("could not open object file");
    auto symbols = object->dynamic_symbols();
    for (auto &symbol : symbols)
      symbol.name(dehash(symbol.name()));
    object->write(out_path);
  }
};

void usage(int code) {
  // clang-format off
  std::cout << "Usage: symbol-slasher hash|slash <arguments>" << std::endl;
  std::cout << std::endl;
  std::cout << "  symbol-slasher insert <store-path> <object-path...>" << std::endl;
  std::cout << "    Hashes the dynamic symbol names in each object provided by <object-path...>" << std::endl;
  std::cout << "    and updates the table stored in <store-path>.  If the hash store does not exist," << std::endl;
  std::cout << "    it is created." << std::endl;
  std::cout << std::endl;
  std::cout << "  symbol-slasher hash <store-path> <input-object-path> <output-object-path>"
            << std::endl;
  std::cout << "    Replaces all of the symbol names in <input-object-path> with the hashes in" << std::endl;
  std::cout << "    <store-path> and writes the modified object to <output-object-path>." << std::endl;
  std::cout << std::endl;
  std::cout << "  symbol-slasher dehash <store-path> <input-object-path> <output-object-path>"
            << std::endl;
  std::cout << "    Replaces all of the hashed symbol names in <input-object-path> with original names in" << std::endl;
  std::cout << "    <store-path> and writes the modified object to <output-object-path>." << std::endl;
  // clang-format on
  std::exit(code);
}

int main(int argc, char **argv) try {
  if (argc < 2)
    usage(-1);

  std::string mode(argv[1]);
  if (mode == "insert") {
    if (argc < 4)
      usage(-1);
    Inserter inserter;
    inserter.open(argv[2]);
    for (int i = 3; i < argc; i++)
      inserter(argv[i]);
  } else if (mode == "hash") {
    if (argc != 5)
      usage(-1);
    Hasher hasher;
    hasher.open(argv[2]);
    hasher(argv[3], argv[4]);
  } else if (mode == "dehash") {
    if (argc != 5)
      usage(-1);
    Dehasher dehasher;
    dehasher.open(argv[2]);
    dehasher(argv[3], argv[4]);
  }
  return 0;
} catch (const std::exception &e) {
  std::cerr << "Error: " << e.what() << std::endl;
  return -1;
}
