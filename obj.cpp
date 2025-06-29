#include "obj.hpp"
#include <vector>
#include <iostream>
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <float.h>


#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

MeshSettings ObjLoader::load_obj(int argc, char **argv) {
  uint32_t count = 0; 
  while(argv[++count] != NULL);
  
  if ((argc == 1) || count > 3 && argv[1][0] == '-') {
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
      std::cout << RIGHT_KEY << std::endl;
      std::cout << LIGHT_KEY << std::endl;
      std::cout << TEX_ORTHO << std::endl;
      std::cout << TEX_CIL << std::endl;
      std::cout << TEX_SPH << std::endl << std::endl;
      std::cout << KEYS << std::endl;
    } break;
    case 'h':
    default: {
      	  std::cout << "para executar o mesh passe um arquivo .obj e uma textura: " << std::endl;
	  std::cout << "./mesh cube.obj noteblock.png" << std::endl << std::endl;
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


  std::vector<uint32_t> indices;
  std::vector<Vertex> verts;
  std::map<uint32_t, uint32_t> vertex_idx; // mapea o index do tiny e  -> index do verts

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

	if (vertex_idx.find(idx.vertex_index) == vertex_idx.end()) {

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

	  glm::vec3 normal = glm::vec3(0.f);
	
	  glm::vec4 color = glm::vec4(0.5f, 0.0f, .5f, 1.0f);
	  if (chan == 1) {
	    color = glm::vec4(.2f, .5f, 0.0f, 1.0f);
	  } else if (chan == 2) {
	    color = glm::vec4(0.0f, 0.2f, 1.0f, 1.0f);
	  }
	  chan = (chan + 1) % 3;
	  verts.push_back((Vertex){ .position = position, .normal = normal, .color = color });
	  vertex_idx[idx.vertex_index] = verts.size() - 1;
	  indices.push_back(verts.size() - 1);
	} else {
	  indices.push_back(vertex_idx[idx.vertex_index]);
	}
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
  
  float escala = 1.0f / maior_dim;

  std::cout << escala << std::endl;

  // calculate triangle normal
  for (uint32_t i = 0; i < indices.size(); i += 3) {
    uint64_t i1 = indices[i+0];
    uint64_t i2 = indices[i+1];
    uint64_t i3 = indices[i+2];
    glm::vec3 p1 = glm::vec3(verts[i1].position.x, verts[i1].position.y, verts[i1].position.z);
    glm::vec3 p2 = glm::vec3(verts[i2].position.x, verts[i2].position.y, verts[i2].position.z);
    glm::vec3 p3 = glm::vec3(verts[i3].position.x, verts[i3].position.y, verts[i3].position.z);
    glm::vec3 normal = glm::cross(p2 - p1, p3 - p1);
    
    verts[i1].normal += normal;
    verts[i2].normal += normal;
    verts[i3].normal += normal;
    //std::cout << glm::to_string(normal) << std::endl;
  }

  for (uint32_t i = 0; i < verts.size(); ++i) {
    // normais aculumadas normalizadas
    verts[i].normal = glm::normalize(verts[i].normal);
    // aplica translacao -centro e escala 
    verts[i].position = glm::vec4((glm::vec3(verts[i].position) - center) * escala, 1.0f);
  }
  
  return (MeshSettings){
    .obj_file = argv[1],
    .tex_file = argv[2],
    .resolution = glm::vec2(WIDTH, HEIGHT),
    .mode = FILL_POLYGON,
    .tex_mode = NO_TEX,
    .vertices = verts,
    .t_verts = verts.size(),
    .indices = indices,
    .t_index = indices.size(),
    .center = center,
    .mouse_pos = glm::vec2(0.0f),
    .rotating = false,
    .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
    .translate = glm::vec3(0.0f),
    .scale = glm::vec3(1.0f),
    .scale_factor = 0.01f,
    .color = glm::vec4(0.466f, 0.363f, 0.755f, 1.0f),
    .bg_color = glm::vec4(0.150f, 0.151f, 0.167f, 1.000f),
    .stroke = 1.0f,
    .light = false,
    .camera_position = glm::vec3(0.0f, 0.0f, 3.0f),
    .light_position = glm::vec3(1.0f, 0.0f, 2.0f),
    .light_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    .ka = 0.5f,
    .kd = 0.8f,
    .ks = 1.0f,
    .ksb = 3.0f,
  };
}
