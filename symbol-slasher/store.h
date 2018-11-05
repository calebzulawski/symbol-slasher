/* store.h
Copyright (C) 2018 Caleb Zulawski

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

#ifndef SYMBOL_SLASHER_STORE_H_
#define SYMBOL_SLASHER_STORE_H_

#include <LIEF/ELF/Parser.hpp>
#include <fstream>
#include <string>
#include <unordered_map>

namespace slasher {

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
    std::unique_ptr<LIEF::ELF::Binary> object;
    try {
      object = LIEF::ELF::Parser::parse(path);
    } catch (...) {
      throw std::logic_error("Could not open object file for reading");
    }
    auto symbols = object->dynamic_symbols();
    for (auto &symbol : symbols) {
      if (symbol.value() != 0)
        insert(symbol.name());
    }
  }
};

struct Hasher : public Forward_map {
  Hasher(bool keep_static) : Forward_map(true), keep_static(keep_static) {}

  void operator()(std::string in_path, std::string out_path) {
    std::unique_ptr<LIEF::ELF::Binary> object;
    try {
      object = LIEF::ELF::Parser::parse(in_path);
    } catch (...) {
      throw std::logic_error("Could not open object file for reading");
    }
    if (!object)
      throw std::logic_error("Could not open object file for reading");
    for (auto &symbol : object->dynamic_symbols())
      symbol.name(hash(symbol.name()));
    if (!keep_static)
      object->remove(object->static_symbols_section(), true);
    try {
      object->write(out_path);
    } catch (...) {
      throw std::logic_error("Could not open object file for writing");
    }
  }

private:
  bool keep_static;
};

struct Dehasher : public Reverse_map {
  void operator()(std::string in_path, std::string out_path) {
    std::unique_ptr<LIEF::ELF::Binary> object;
    try {
      object = LIEF::ELF::Parser::parse(in_path);
    } catch (...) {
      throw std::logic_error("Could not open object file for reading");
    }
    if (!object)
      throw std::logic_error("Could not open object file for reading");
    auto symbols = object->dynamic_symbols();
    for (auto &symbol : symbols)
      symbol.name(dehash(symbol.name()));
    try {
      object->write(out_path);
    } catch (...) {
      throw std::logic_error("Could not open object file for writing");
    }
  }
};

} // namespace slasher

#endif // SYMBOL_SLASHER_STORE_H_
