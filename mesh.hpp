#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>

#include <vector>

#define MOUSE_ICON_FILE "mouse_icon.png"

#define WIDTH 860
#define HEIGHT 640

#define EXIT_KEY "(q/esq): termina a execução do programa mesh."
#define V_KEY "(v): troca o modo de visualização de fill para line (wireframe)."
#define W_KEY "(w): faz deslocamento positivo em z."
#define S_KEY "(s): faz deslocamento negativo em z."
#define DOWN_KEY "(seta para baixo): faz deslocamento negativo em y."
#define UP_KEY "(seta para cima): faz deslocamento postivo em y."
#define LEFT_KEY "(seta para esquerda): faz deslocamento negativo em x."
#define RIGHT_KEY "(seta para direita): faz deslocamento positivo em x."
#define K_KEY "(k): abre/fecha a tela de controles."
#define KEYS "para ler novamente passe a opção -k ou acesse a tela de controles."

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
  glm::vec3 translate;
  glm::vec3 scale;
  float scale_factor;
  float angle;
  glm::vec3 axis;
  glm::vec3 color;
  float blend;
  float stroke;
} MeshSettings;

void show_global_info();
void show_global_settings(MeshSettings *mesh_set);
void show_model_matrix(MeshSettings *mesh_set);
void show_color_blend(MeshSettings *mesh_set);
void show_controls(bool *p_open);
#endif /* MESH_H */
