#include "obj.hpp"
#include "rapidobj.hpp"

#include <iostream>

rapidobj::Result ObjLoader::load_obj(int argc, char **argv) {

  if (argc != 2) {
    std::cout << "passe 1 arquivo .obj como argumento" << std::endl;
    exit(1);
  }
  
  rapidobj::Result result = rapidobj::ParseFile(argv[1]);
  if (result.error) {
    std::cout << result.error.code.message() << std::endl;
    exit(1);
  }
  bool success = rapidobj::Triangulate(result);

  if (!success) {
    std::cout << result.error.code.message() << std::endl;
    exit(1);
  }
  
  return result;
}
