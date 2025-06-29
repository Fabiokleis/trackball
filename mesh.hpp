#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

#define MOUSE_ICON_FILE "mouse_icon.png"

#define WIDTH 1280
#define HEIGHT 720

#define EXIT_KEY "(q/esq): termina a execução do programa mesh."
#define V_KEY "(v): troca o modo de visualização de fill para line (wireframe)."
#define W_KEY "(w): faz deslocamento positivo em z."
#define S_KEY "(s): faz deslocamento negativo em z."
#define DOWN_KEY "(seta para baixo): faz deslocamento negativo em y."
#define UP_KEY "(seta para cima): faz deslocamento postivo em y."
#define LEFT_KEY "(seta para esquerda): faz deslocamento negativo em x."
#define RIGHT_KEY "(seta para direita): faz deslocamento positivo em x."
#define LIGHT_KEY "(1): habilita/desabilita o modo de iluminação phong."
#define TEX_ORTHO "(2): habilita/desabilita o mapeamento de textura com projeção paralela sobre a imagem da textura."
#define TEX_CIL "(3): habilita/desabilita o mapeamento de textura com coordenadas cilíndricas."
#define TEX_SPH "(4): habilita/desabilita o mapeamento de textura com coordenadas esféricas."
#define K_KEY "(k): abre/fecha a tela de controles."
#define KEYS "para ler novamente passe a opção -k ou acesse a tela de controles."

typedef struct {
  glm::vec4 position;
  glm::vec3 normal;
  glm::vec4 color;
} Vertex;

enum VISUALIZATION_MODE {
  FILL_POLYGON,
  WIREFRAME,
};

enum TEXTURE_MODE {
  NO_TEX = 0,
  ORTHO,
  CIL,
  SPH,
};

typedef struct {
  const char *obj_file;
  const char *tex_file;
  glm::vec2 resolution; 
  VISUALIZATION_MODE mode;
  TEXTURE_MODE tex_mode;
  std::vector<Vertex> vertices;
  uint64_t t_verts;
  std::vector<uint32_t> indices;
  uint64_t t_index;
  glm::vec3 center;
  glm::vec2 mouse_pos;
  bool rotating;
  glm::quat rotation;
  glm::vec3 translate;
  glm::vec3 scale;
  float scale_factor;
  glm::vec3 color;
  glm::vec3 bg_color;
  float stroke;
  bool light;
  glm::vec3 camera_position;
  glm::vec3 light_position;
  glm::vec3 light_color;
  float ka;
  float kd;
  float ks;
  float ksb;
} MeshSettings;

void show_global_info(MeshSettings *mesh_set);
void show_global_settings(MeshSettings *mesh_set);
void show_model_matrix(MeshSettings *mesh_set);
void show_lightning(MeshSettings *mesh_set);
void show_controls(bool *p_open);

#endif /* MESH_H */
