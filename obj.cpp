#include "obj.hpp"
#include "rapidobj.hpp"

#include <iostream>

void ObjLoader::load_obj(int argc, char **argv) {

  if (argc != 2) {
    std::cout << "passe 1 arquivo .obj como argumento" << std::endl;
    exit(1);
  }
  
  rapidobj::Result result = rapidobj::ParseFile(argv[1]);
  if (result.error) {
    std::cout << result.error.code.message() << std::endl;
    exit(1);
  }
  // bool success = rapidobj::Triangulate(result);

  // if (!success) {
  //   std::cout << result.error.code.message() << std::endl;
  //   exit(1);
  // }

  size_t num_triangles{};

  for (const auto& shape : result.shapes) {
    num_triangles += shape.mesh.num_face_vertices.size();
  }

  std::cout << "Shapes:    " << result.shapes.size() << std::endl;
  std::cout << "Triangles: " << num_triangles << std::endl;
}
