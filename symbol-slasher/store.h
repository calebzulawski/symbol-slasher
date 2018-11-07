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
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

using json = nlohmann::json;

namespace slasher {

constexpr auto prefix = "symslash";

struct Store_base {
  Store_base(bool read_only) : read_only(read_only) {}

  void open(std::filesystem::path store_path) {
    this->store_path = store_path;
    std::ifstream store_stream(store_path);

    if (store_stream.is_open() && store_stream.good()) {
      if (store_stream.peek() != std::ifstream::traits_type::eof()) {
        json store;
        store_stream >> store;
        for (auto &symbol : store["symbols"])
          insert(symbol["name"], symbol["hash"]);
      }
    } else if (read_only) {
      throw std::logic_error("failed to open hash store");
    }
  }

  virtual ~Store_base(){};

protected:
  std::filesystem::path store_path;

  const bool read_only;

private:
  virtual void insert(std::string name, uint64_t hash){};
};

struct Forward_map : public Store_base {
  Forward_map(bool read_only) : Store_base(read_only) {}

  ~Forward_map() {
    // Dump only new hashes
    if (!read_only) {
      std::ofstream store_stream(store_path);
      json store;
      for (const auto &[name, hash] : symbol_map) {
        json symbol;
        symbol["name"] = name;
        symbol["hash"] = hash;
        store["symbols"].push_back(symbol);
      }
      store >> store_stream;
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

std::unique_ptr<LIEF::ELF::Binary>
load_binary(std::filesystem::path object_path) {
  std::unique_ptr<LIEF::ELF::Binary> object;
  try {
    object = LIEF::ELF::Parser::parse(object_path);
  } catch (...) {
    throw std::logic_error("Could not open object file for reading");
  }
  if (!object)
    throw std::logic_error("Could not open object file for reading");
  return object;
}

void store_binary(std::filesystem::path in_path, std::filesystem::path out_path,
                  std::unique_ptr<LIEF::ELF::Binary> &object) {
  try {
    object->write(out_path);
  } catch (...) {
    throw std::logic_error("Could not open object file for writing");
  }

  // Copy file permissions
  std::filesystem::permissions(out_path,
                               std::filesystem::status(in_path).permissions());
}

struct Inserter : public Forward_map {
  Inserter() : Forward_map(false) {}

  void operator()(std::filesystem::path object_path) {
    auto object = load_binary(object_path);
    auto symbols = object->dynamic_symbols();
    for (auto &symbol : symbols) {
      if (symbol.value() != 0)
        insert(symbol.name());
    }
  }
};

struct Hasher : public Forward_map {
  Hasher(bool keep_static) : Forward_map(true), keep_static(keep_static) {}

  void operator()(std::filesystem::path in_path,
                  std::filesystem::path out_path) {
    auto object = load_binary(in_path);
    for (auto &symbol : object->dynamic_symbols())
      symbol.name(hash(symbol.name()));
    if (!keep_static)
      object->remove(object->static_symbols_section(), true);
    store_binary(in_path, out_path, object);
  }

private:
  bool keep_static;
};

struct Dehasher : public Reverse_map {
  void operator()(std::string in_path, std::string out_path) {
    auto object = load_binary(in_path);
    auto symbols = object->dynamic_symbols();
    for (auto &symbol : symbols)
      symbol.name(dehash(symbol.name()));
    store_binary(in_path, out_path, object);
  }
};

} // namespace slasher

#endif // SYMBOL_SLASHER_STORE_H_
