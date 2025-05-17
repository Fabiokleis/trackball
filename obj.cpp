#include "obj.hpp"
#include <vector>
#include <iostream>

#include <glm/gtx/string_cast.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <float.h>


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

  float min_x = FLT_MAX;
  float max_x = FLT_MIN;
  float min_y = FLT_MAX;
  float max_y = FLT_MIN;
  float min_z = FLT_MAX;
  float max_z = FLT_MIN;

  std::vector<Vertex> verts;

  uint32_t chan = 0;
  // Loop over shapes
  for (uint32_t s = 0; s < shapes.size(); s++) {
    // Loop over faces(polygon)
    uint32_t index_offset = 0;
    for (uint32_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      uint32_t fv = (uint32_t)(shapes[s].mesh.num_face_vertices[f]);

      // Loop over vertices in the face.
      for (uint32_t v = 0; v < fv; v++) {
	// access to vertex
	tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
	tinyobj::real_t vx = attrib.vertices[3*(uint32_t)(idx.vertex_index)+0];
	tinyobj::real_t vy = attrib.vertices[3*(uint32_t)(idx.vertex_index)+1];
	tinyobj::real_t vz = attrib.vertices[3*(uint32_t)(idx.vertex_index)+2];

	min_x = std::min(min_x, vx);
	max_x = std::max(max_x, vx);
	min_y = std::min(min_y, vy);
	max_y = std::max(max_y, vy);
	min_z = std::min(min_z, vz);
	max_z = std::max(max_z, vz);
	
	glm::vec4 position = glm::vec4(vx, vy, vz, 1.0f);

	glm::vec4 color = glm::vec4(0.5f, 0.0f, .5f, 1.0f);
	if (chan == 1) {
	  color = glm::vec4(.2f, .5f, 0.0f, 1.0f);
	} else if (chan == 2) {
	  color = glm::vec4(0.0f, 0.2f, 1.0f, 1.0f);
	}
	chan = (chan + 1) % 3;
	verts.push_back((Vertex){ .position = position, .color = color });
      }
      index_offset += fv;

      // per-face material
      //shapes[s].mesh.material_ids[f];
    }
  }

  glm::vec3 center = glm::vec3(0.0f);

  center.x = (min_x + max_x) / 2.0f;
  center.y = (min_y + max_y) / 2.0f;
  center.z = (min_z + max_z) / 2.0f;

  float tam_x = max_x - min_x;
  float tam_y = max_y - min_y;
  float tam_z = max_z - min_z;

  float maior_dim = std::max(std::max(tam_x, tam_y), tam_z);
  
  float escala = 2.0f / maior_dim;
  
  return (MeshSettings){
    .obj_file = argv[1],
    .resolution = glm::vec2(WIDTH, HEIGHT),
    .mode = FILL_POLYGON,
    .vertices = verts,
    .t_verts = verts.size(),
    .center = center,
    .mouse_pos = glm::vec2(0.0f),
    .rotating = false,
    .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
    .translate = glm::vec3(0.0f),
    .scale = glm::vec3(escala),
    .scale_factor = 0.01f,
    .angle = glm::vec2(0.0f, 0.0f),
    .color = glm::vec4(0.466f, 0.363f, 0.755f, 1.0f),
    .blend = 0.5f,
    .stroke = 1.0f
  };
}
