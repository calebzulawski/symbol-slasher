project('symbol-slasher', 'cpp')
cxx = meson.get_compiler('cpp')
bfd = cxx.find_library('libbfd')
main = executable('symbol-slasher', 'slasher.cpp', dependencies: bfd)
