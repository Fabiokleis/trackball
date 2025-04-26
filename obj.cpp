#include "obj.hpp"
#include <vector>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

MeshSettings ObjLoader::load_obj(int argc, char **argv) {
    
  if (argc != 2) {
    std::cout << "passe 1 arquivo .obj como argumento." << std::endl;
    exit(1);
  }
  
  tinyobj::ObjReaderConfig reader_config;
  reader_config.mtl_search_path = "./models/"; // Path to material files

  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(argv[1], reader_config)) {
    if (!reader.Error().empty()) {
      std::cerr << "TinyObjReader: " << reader.Error();
    }
    exit(1);
  }

  if (!reader.Warning().empty()) {
    std::cout << "TinyObjReader: " << reader.Warning();
  }

  auto& attrib = reader.GetAttrib();
  auto& shapes = reader.GetShapes();
  auto& materials = reader.GetMaterials();

  if (shapes.size() < 1) {
    std::cout << "precisa de pelo menos 1 shape." << std::endl;
    exit(1);
  }
  std::vector<Vertex> verts;
  // Loop over shapes
  for (size_t s = 0; s < shapes.size(); s++) {
    // Loop over faces(polygon)
    size_t index_offset = 0;
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

      // Loop over vertices in the face.
      for (size_t v = 0; v < fv; v++) {
	// access to vertex
	tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
	tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
	tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
	tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];
	glm::vec4 position = glm::vec4(vx, vy, vz, 1.0f);

	// // Check if `normal_index` is zero or positive. negative = no normal data
	// if (idx.normal_index >= 0) {
	//   tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
	//   tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
	//   tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
	// }

	// // Check if `texcoord_index` is zero or positive. negative = no texcoord data
	// if (idx.texcoord_index >= 0) {
	//   tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
	//   tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
	// }

	glm::vec4 color = glm::vec4(1.0f, 0.5f, 1.0f, 1.0f);
	// Optional: vertex colors
	if (!attrib.colors.empty()) {
	  tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
	  tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
	  tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
	  color = glm::vec4(red, green, blue, 1.0f);
	}

	verts.push_back((Vertex){ .position = position, .color = color });
      }
      index_offset += fv;

      // per-face material
      shapes[s].mesh.material_ids[f];
    }
  }

  MeshSettings mesh_set = (MeshSettings){
    .obj_file = argv[1],
    .mode = FILL_POLYGON,
    .vertices = verts,
    .t_verts = verts.size(),
    .indices = std::vector<uint32_t>(),
    .t_idx = 0,
    .translate = glm::vec3(0.0f),
    .scale = glm::vec3(0.5f),
    .angle = 0.0f,
    .axis = glm::vec3(1.0f)
  };
  
  return mesh_set;
}
