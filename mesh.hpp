#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>

#include <vector>
typedef struct {
  glm::vec4 position;
  glm::vec4 color;
} Vertex;

enum VISUALIZATION_MODE {
  FILL_POLYGON,
  WIREFRAME,
};

typedef struct {
  const char *obj_file;
  VISUALIZATION_MODE mode;
  std::vector<Vertex> vertices;
  uint64_t t_verts;
  std::vector<uint32_t> indices;
  uint64_t t_idx;
  glm::vec3 translate;
  glm::vec3 scale;
  float angle;
  glm::vec3 axis;
} MeshSettings;

void show_global_info();
void show_global_settings(MeshSettings *mesh_set);
void show_model_matrix(MeshSettings *mesh_set);

#endif /* MESH_H */
