project('example', 'cpp')
alib = shared_library('a', 'a.cpp')
blib = shared_library('b', 'b.cpp', link_with: alib)
main = executable('main', 'main.cpp', link_with: [alib, blib])
