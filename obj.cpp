#include "obj.hpp"
#include <vector>
#include <iostream>
#include <glm/ext/matrix_transform.hpp>

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

MeshSettings ObjLoader::load_obj(int argc, char **argv) {
  uint32_t count = 0; 
  while(argv[++count] != NULL);
  
  if (argc != 2 || (count > 2 && argv[1][0] == '-')) {
    std::cerr << "opção inválida, passe -h para mostrar a mensagem de help." << std::endl;
    exit(1);
  }

  if (argc == 2 && (count && argv[1][0] == '-')) {
    switch (argv[1][1]) {
    case 'k': {
      std::cout << "controles disponiveis: " << std::endl << std::endl;
      std::cout << EXIT_KEY << std::endl;
      std::cout << K_KEY << std::endl;
      std::cout << V_KEY << std::endl;
      std::cout << W_KEY << std::endl;
      std::cout << S_KEY << std::endl;
      std::cout << DOWN_KEY << std::endl;
      std::cout << UP_KEY << std::endl;
      std::cout << LEFT_KEY << std::endl;
      std::cout << RIGHT_KEY << std::endl << std::endl;
      std::cout << "para ler novamente passe a opção -k ou acesse a tela de controles." << std::endl;
    } break;
    case 'h':
    default: {
      	  std::cout << "para executar o mesh passe um arquivo .obj: " << std::endl;
	  std::cout << "./mesh cube.obj" << std::endl << std::endl;
      	  std::cout << "opções: " << std::endl;
	  std::cout << "-h: mostra essa mensagem." << std::endl;
	  std::cout << "-k: mostra a mensagem de controles." << std::endl;
    } break;
    }
    exit(0);
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

  uint32_t chan = 0;
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
	// std::cout << "x: " << vx << std::endl;
	// std::cout << "y: " << vy << std::endl;
	// std::cout << "z: " << vz << std::endl;
	// std::cout << "w: " << position.w << std::endl;

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
	glm::vec4 color = glm::vec4(0.5f, 0.0f, .5f, 1.0f);
	if (chan == 1) {
	  color = glm::vec4(.2f, .5f, 0.0f, 1.0f);
	} else if (chan == 2) {
	  color = glm::vec4(0.0f, 0.2f, 1.0f, 1.0f);
	}
	chan = (chan + 1) % 3;
	// Optional: vertex colors
	// if (!attrib.colors.empty()) {
	//   tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
	//   tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
	//   tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
	//   color = glm::vec4(red, green, blue, 1.0f);
	// }
	verts.push_back((Vertex){ .position = position, .color = color });
      }
      index_offset += fv;

      // per-face material
      //shapes[s].mesh.material_ids[f];
    }
  }

  // for (const auto &v : verts) {
  //   std::cout << "x: " << v.position.x << std::endl;
  //   std::cout << "y: " << v.position.y << std::endl;
  //   std::cout << "z: " << v.position.z << std::endl;
  //   std::cout << "w: " << v.position.w << std::endl;
  // }

  return (MeshSettings){
    .obj_file = argv[1],
    .mode = FILL_POLYGON,
    .vertices = verts,
    .t_verts = verts.size(),
    .translate = glm::vec3(0.0f),
    .scale = glm::vec3(0.5f),
    .scale_factor = 0.05f,
    .angle = 0.0f,
    .axis = glm::vec3(1.0f),
    .color = glm::vec4(0.466f, 0.363f, 0.755f, 1.0f),
    .blend = 0.5f,
    .stroke = 1.0f
  };
}
