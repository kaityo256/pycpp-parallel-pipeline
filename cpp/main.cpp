#include <cstdio>
#include <iostream>
#include <param.hpp>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Error: Input file is not specified.\n";
    std::cerr << "Usage: " << argv[0] << " <input_file>\n";
    return 1;
  }
  const std::string filename = argv[1];

  param::parameter param(filename);

  if (!param) {
    std::cerr << "Error: Failed to open input file: "
              << filename << '\n';
    return 1;
  }
}