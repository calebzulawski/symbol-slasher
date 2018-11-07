# Symbol Slasher

Symbol Slasher renames dynamic symbols on already-linked binaries.
This obfuscates the interactions with private libraries, or simply prevents future linking to those libraries.

## Building
Install the following:
* [libLIEF](https://github.com/lief-project/lief)
* [nlohmann/json](https://github.com/nlohmann/json)

To install on Ubuntu:
```bash
sudo apt install liblief-dev nlohmann-json-dev
```
Then run:
```bash
meson symbol-slasher build
cd build && ninja
```
## Example
This section assumes you have built the example libraries and executable in `/example`.

Example library `liba.so` contains a simple print function.  Example library `libb.so` contains its own print function, plus a wrapper for `liba.so`'s print function.  The executable `main` calls all three print functions.  Some snippets from `nm -DC`:
```
liba.so:
00000000000011a5 T a::print(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)

libb.so:
                 U a::print(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
0000000000001252 T b::a::print(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
00000000000011d5 T b::print(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)

main:
                 U a::print(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
                 U b::a::print(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
                 U b::print(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
```

To build the symbol hash table, run:
```
symbol-slasher insert liba.so libb.so
```
Note that `main` does not need to be inserted into the table, since only *defined* symbols are inserted into the table (since there will always be some undefined symbols that are resolved by `libstdc++`, for example).

To create hash the symbol names, run:
```
mkdir hashed
symbol-slasher hash liba.so hashed/liba.so
symbol-slasher hash libb.so hashed/libb.so
symbol-slasher hash main hashed/main
```

Some snippets from `nm -DC` on the hashed binaries:
```
liba.so
00000000000011a5 T symslash2

libb.so:
00000000000011d5 T symslash0
0000000000001252 T symslash1
                 U symslash2
                 
main:
                 U symslash0
                 U symslash1
                 U symslash2
```
