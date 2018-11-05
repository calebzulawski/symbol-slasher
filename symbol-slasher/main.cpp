/* main.cpp
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

#include "cxxopts.hpp"
#include "store.h"
#include <cstdlib>
#include <iostream>
#include <string>

constexpr auto insert_desc =
    "Parses objects and appends their hashed symbol names to the symbol store.";
constexpr auto hash_desc = "Replaces the symbol names in an object with the "
                           "hashed equivalent from the symbol store.";
constexpr auto dehash_desc = "Replaces hashed symbol names in an object with "
                             "the original name from the symbol store.";

int insert(int argc, char **argv) {
  std::string store_path;
  std::vector<std::string> object_paths;
  cxxopts::Options options("symbol-slasher insert", insert_desc);
  // clang-format off
  options.add_options()
      ("h,help", "print this help")
      ("s,symbols", "path to the store of symbol hashes", cxxopts::value(store_path)->default_value("symbol_hashes"))
      ("object_paths", "paths of objects to read", cxxopts::value(object_paths))
      ;
  // clang-format on
  options.parse_positional({"object_paths"});
  options.positional_help("object_path(s)...");
  auto args = options.parse(argc, argv);

  if (args.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  slasher::Inserter inserter;
  inserter.open(store_path.c_str());
  for (const auto &object_path : object_paths)
    inserter(object_path);
  return 0;
}

int hash(int argc, char **argv) {
  std::string store_path;
  std::string input_object_path;
  std::string output_object_path;
  cxxopts::Options options("symbol-slasher hash", hash_desc);
  // clang-format off
  options.add_options()
      ("h,help", "print this help")
      ("s,symbols", "path to the store of symbol hashes", cxxopts::value(store_path)->default_value("symbol_hashes"))
      ("k,keep-static", "do not discard static symbols")
      ("i,input-object-path", "object to read", cxxopts::value(input_object_path))
      ("o,output-object-path", "new object to create", cxxopts::value(output_object_path))
      ;
  // clang-format on
  options.parse_positional({"input-object-path", "output-object-path"});
  options.positional_help("input-object-path output-object-path");
  auto args = options.parse(argc, argv);

  if (args.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  slasher::Hasher hasher(args.count("keep-static"));
  hasher.open(store_path.c_str());
  hasher(input_object_path, output_object_path);
  return 0;
}

int dehash(int argc, char **argv) {
  std::string store_path;
  std::string input_object_path;
  std::string output_object_path;
  cxxopts::Options options("symbol-slasher hash", hash_desc);
  // clang-format off
  options.add_options()
      ("h,help", "print this help")
      ("s,symbols", "path to the store of symbol hashes", cxxopts::value(store_path)->default_value("symbol_hashes"))
      ("i,input_object_path", "object to read", cxxopts::value(input_object_path))
      ("o,output_object_path", "new object to create", cxxopts::value(output_object_path))
      ;
  // clang-format on
  options.parse_positional({"input_object_path", "output_object_path"});
  options.positional_help("input_object_path output_object_path");
  auto args = options.parse(argc, argv);

  if (args.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  slasher::Dehasher dehasher;
  dehasher.open(store_path.c_str());
  dehasher(input_object_path, output_object_path);
  return 0;
}

void help() {
  // clang-format off
  std::cout << "Symbol Slasher: obfuscate shared object libraries by hashing symbol names." << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << "  symbol-slasher -h | --help            Print this help." << std::endl;
  std::cout << "  symbol-slasher <command> -h | --help  Print command help." << std::endl;
  std::cout << std::endl;
  std::cout << "Commands:" << std::endl;
  std::cout << "  insert   " << insert_desc << std::endl;
  std::cout << "  hash     " << hash_desc << std::endl;
  std::cout << "  dehash   " << dehash_desc << std::endl;
  // clang-format on
  std::exit(0);
}

int main(int argc, char **argv) try {
  if (argc < 2)
    help();
  std::string mode(argv[1]);
  auto call_mode = [&](auto func) { std::exit(func(argc - 1, argv + 1)); };
  if (mode == "-h" || mode == "--help") {
    help();
  } else if (mode == "insert") {
    call_mode(insert);
  } else if (mode == "hash") {
    call_mode(hash);
  } else if (mode == "dehash") {
    call_mode(dehash);
  } else {
    throw std::logic_error("invalid command");
  }

} catch (const std::exception &e) {
  std::cerr << "Error: " << e.what() << std::endl;
  std::cerr << "See 'symbol-slasher -h' for more information." << std::endl;
  return -1;
}
