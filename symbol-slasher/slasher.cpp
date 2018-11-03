extern "C" {
#include <bfd.h>
}

#include <iostream>

struct Bfd_exception : std::exception {
  Bfd_exception() : msg(bfd_errmsg(bfd_get_error())) {}
  const char *what() const noexcept { return msg; }

private:
  const char *msg = nullptr;
};

struct Object {
  Object(std::string path) : descriptor(bfd_openr(path.c_str(), nullptr)) {
    if (descriptor == nullptr)
      throw Bfd_exception();

    if (!bfd_check_format(descriptor, bfd_object))
      throw Bfd_exception();

    auto symbol_size = bfd_get_dynamic_symtab_upper_bound(descriptor);
    if (symbol_size < 0)
      throw Bfd_exception();
    symbols = (bfd_symbol **)new char[symbol_size];

    auto symbol_count = bfd_canonicalize_dynamic_symtab(descriptor, symbols);
    if (symbol_count < 0)
      throw Bfd_exception();

    for (int i = 0; i < symbol_count; i++) {
      if (symbols[i]->value != 0)
        std::cout << symbols[i]->name << std::endl;
    }
  }

  ~Object() {
    bfd_close(descriptor);
    delete symbols;
  }

private:
  bfd *descriptor;
  bfd_symbol **symbols;
};

int slash(std::string path) try {
  bfd_init();
  Object obj(path);
  return 0;
} catch (const Bfd_exception &e) {
  std::cerr << "libbfd error: " << e.what() << std::endl;
  return -1;
} catch (const std::exception &e) {
  std::cerr << "exception: " << e.what() << std::endl;
  return -1;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return -1;
  std::string path = argv[1];
  return slash(path);
}
