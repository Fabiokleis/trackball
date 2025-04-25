#include <iostream>
#include <cstdint>
#include <string.h>
#include <errno.h>
#include <chrono>
#include <thread>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define MOUSE_ICON_FILE "mouse_icon.png"

#define WIDTH 860
#define HEIGHT 640

#define MAX_TRIANGLES 1000
#define MAX_VERTEX_COUNT MAX_TRIANGLES * 3
#define MAX_IDX_COUNT MAX_TRIANGLES * 3

#include "obj.hpp"

const static char *vertex_shader_source = R"(
  #version 330 core
  layout (location = 0) in vec4 v_pos;
  layout (location = 1) in vec4 v_color;
  uniform mat4 v_model;
  uniform mat4 v_view;
  uniform mat4 v_projection;
  out vec4 color;
  void main() {
     gl_Position = v_projection * v_view * v_model * v_pos;
     color = v_color;
  };
)";

// (color * v_color) * v_time
const static char *fragment_shader_source = R"(
  #version 330 core
  in vec4 color;
  uniform float v_time;
  out vec4 FragColor;
  void main()
  {
     vec4 c = color * (sin(v_time) / cos(v_time));
     FragColor = vec4(c.x * sin(v_time), c.y, c.z * cos(v_time), 1.0f);
  };
)";

int compile_shaders(uint32_t *shader_program) {

  // vertex shader
  unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  // check for shader compile errors
  int success;
  char infoLog[512];
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success)
    {
      glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
      return -1;
    }
  // fragment shader
  uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);
  // check for shader compile errors
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success)
    {
      glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
      return -1;
    }
  // link shaders
  *shader_program = glCreateProgram();
  glAttachShader(*shader_program, vertex_shader);
  glAttachShader(*shader_program, fragment_shader);
  glLinkProgram(*shader_program);
  // check for linking errors
  glGetProgramiv(*shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(*shader_program, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    return -1;
  }
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  return 0;
}

typedef struct {
  double x, y;
} Vec2;

bool is_key_pressed(GLFWwindow *window, int keycode) {
    int state = glfwGetKey(window, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool is_mouse_button_pressed(GLFWwindow *window, int button) {
    int state = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS;
}

bool should_quit(GLFWwindow *window) {
  return is_key_pressed(window, GLFW_KEY_ESCAPE) || is_key_pressed(window, GLFW_KEY_Q) || glfwWindowShouldClose(window);
}

void resize_callback(GLFWwindow* window, int width, int height) {
  glfwGetWindowSize(window, &width, &height);
  glViewport(0, 0, width, height);
}

glm::vec3 scale = glm::vec3(.5f);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
  scale += (yoffset * .05f);
  std::cout << "scale: " << glm::to_string(scale) << std::endl;
}

Vec2 get_mouse_pos(GLFWwindow *window) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return Vec2{ .x = xpos, .y = ypos };
}

typedef struct {
  float x, y, z, w;
} Position;

typedef struct {
  float r, g, b, a;
} Color;

typedef struct {
  Position position;
  Color color;
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

uint32_t put_vertice(uint32_t idx, Vertex vertices[MAX_VERTEX_COUNT], Position pos, Color color) {
  vertices[idx].position = pos;
  vertices[idx].color = color;
  //vertices[idx].size = 10.0f;
  return idx;
}

// mouse offset 1 -1
glm::vec3 mouse_to_gl_point(float x, float y) {
  return glm::vec3((2.0f * x) / WIDTH - 1.0f, 1.0f - (2.0f * y) / HEIGHT, 0.0f);
}

void draw(uint32_t VAO, uint32_t program, MeshSettings mesh_set) {
  float time = (float)glfwGetTime();
  glm::mat4 view = glm::mat4(1.0f);
  view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
  glm::mat4 projection = glm::mat4(1.0f);

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, mesh_set.translate);
  model = glm::rotate(model, glm::radians(mesh_set.angle * time), mesh_set.axis);
  model = glm::scale(model, mesh_set.scale);

  projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

  int v_model = glGetUniformLocation(program, "v_model");
  int v_view = glGetUniformLocation(program, "v_view");
  int v_projection = glGetUniformLocation(program, "v_projection");
  int v_time = glGetUniformLocation(program, "v_time");
  glUniformMatrix4fv(v_model, 1, GL_FALSE, &model[0][0]);
  glUniformMatrix4fv(v_view, 1, GL_FALSE, &view[0][0]);
  glUniformMatrix4fv(v_projection, 1, GL_FALSE, &projection[0][0]);
  glUniform1f(v_time, time);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, mesh_set.t_idx, GL_UNSIGNED_INT, 0);
  //glDrawArrays(GL_TRIANGLES, 0, mesh_set.t_verts);
}

static void show_global_info() {
  ImGuiIO& io = ImGui::GetIO();
  static int location = -1;
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

  if (location >= 0) {
    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos, window_pos_pivot;
    window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
    window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
    window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
    window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    window_flags |= ImGuiWindowFlags_NoMove;
  } else if (location == -2) {
    // Center window
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    window_flags |= ImGuiWindowFlags_NoMove;
  }
      
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  if (ImGui::Begin("info", nullptr, window_flags)) {
      ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::Separator();
      if (ImGui::IsMousePosValid())
	ImGui::Text("Posição do mouse: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
      else
	ImGui::Text("Posição do mouse: fora da tela");
      if (ImGui::BeginPopupContextWindow()) {
	if (ImGui::MenuItem("Custom",       NULL, location == -1)) location = -1;
	if (ImGui::MenuItem("Center",       NULL, location == -2)) location = -2;
	if (ImGui::MenuItem("Top-left",     NULL, location == 0)) location = 0;
	if (ImGui::MenuItem("Top-right",    NULL, location == 1)) location = 1;
	if (ImGui::MenuItem("Bottom-left",  NULL, location == 2)) location = 2;
	if (ImGui::MenuItem("Bottom-right", NULL, location == 3)) location = 3;
	ImGui::EndPopup();
      }
  }
  ImGui::End();
}

static void show_global_settings(MeshSettings *mesh_set) {
  ImGuiIO& io = ImGui::GetIO(); (void) io;
  static int menu_item = 0;
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav;
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  
  if (ImGui::Begin("mesh", nullptr, window_flags)) {
    ImGui::Text("malha: %s", mesh_set->obj_file);
    ImGui::Text("modo (v): %s", mesh_set->mode == FILL_POLYGON ? "fill polygon" : "polygon wireframe");
    ImGui::Separator();
    ImGui::Text("vertices: %lu", mesh_set->t_verts);
    ImGui::Text("triangulos: %lu", mesh_set->t_idx / 3);
    ImGui::Text("indices: %lu", mesh_set->t_idx);

    if (ImGui::BeginPopupContextWindow()) {
      if (ImGui::MenuItem("trocar modo (v)", NULL, menu_item == 1)) {
	menu_item = 0;
	mesh_set->mode = (VISUALIZATION_MODE)(((uint32_t)mesh_set->mode + 1) % (WIREFRAME + 1));
      }
      ImGui::EndPopup();
    }
  }
  ImGui::End();
}

static void show_model_matrix(MeshSettings *mesh_set) {
  ImGuiIO& io = ImGui::GetIO(); (void) io;
  static int menu_item = 0;
  ImGuiWindowFlags window_flags =  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav;
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  
  if (ImGui::Begin("model", nullptr, window_flags)) {
    ImGui::Separator();
    ImGui::Text("translacao (%.2f, %.2f, %.2f)", mesh_set->translate.x, mesh_set->translate.y, mesh_set->translate.z);
    ImGui::Text("scala      (%.2f, %.2f, %.2f)", mesh_set->scale.x, mesh_set->scale.y, mesh_set->scale.z);
    ImGui::Text("eixo       (%.1f, %.1f, %.1f)", mesh_set->axis.x, mesh_set->axis.y, mesh_set->axis.z);
  }
  ImGui::End();
}

void loop(GLFWwindow *window, MeshSettings mesh_set) {

  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  ImGui::StyleColorsClassic();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init((char *)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  
  int width = 0;
  int height = 0;
  Vec2 mouse_pos = {0};

  int m_width = 0;
  int m_height = 0;
  int channels_in_file = 0;
  
  uint8_t *image_buffer = stbi_load(MOUSE_ICON_FILE, &m_width, &m_height, &channels_in_file, 4);
  if (image_buffer == nullptr) {
    std::cerr << "Could not load image!" << std::endl;
    std::cerr << "STB Image Error: " << stbi_failure_reason() << std::endl;
    std::cerr << "error: " << strerror(errno) << std::endl;
    exit(1);
  }
 
  GLFWimage image;
  image.width = m_width;
  image.height = m_height;
  image.pixels = image_buffer;
  
  GLFWcursor* cursor = glfwCreateCursor(&image, 0, 0);
  if (cursor == nullptr) {
    std::cerr << "Could not create glfw cursor!" << std::endl;
    std::cerr << "error: " << strerror(errno) << std::endl;
    exit(1);
  }

  if (image_buffer) {
    stbi_image_free(image_buffer);
  }
  
  glfwSetCursor(window, cursor);

  bool quit = false;
  glfwSetFramebufferSizeCallback(window, resize_callback);
  glfwSetScrollCallback(window, scroll_callback);

  uint32_t program;
  int error = compile_shaders(&program);
  if (error != 0) exit(1);

  glm::vec3 translate = glm::vec3(0.0f, 0.f, 0.f);
  float angle = 0.0f;

  uint32_t VAO, VBO, EBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);
  
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, mesh_set.t_verts * sizeof(Vertex), &mesh_set.vertices[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_set.t_idx * sizeof(mesh_set.indices[0]), &mesh_set.indices[0], GL_STATIC_DRAW);
  
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
  glEnableVertexAttribArray(0); // location 0

  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
  glEnableVertexAttribArray(1); // location 1

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0); 

  glEnable(GL_DEPTH_TEST);


  float start_time = glfwGetTime();
  float delta = 0.0f;
  float total_time = 0.0f;

  float frame_time = 1.0f / 30.0f;
  
  float click_time = 0.0f;
  float threshold = 0.2f; // mouse

  float key_time = 0.0f;
  float key_threshold = 0.2f;
  uint32_t total_click = 0;
  
  while (!quit) {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    
    delta = glfwGetTime() - start_time;
    total_time += delta;
    if (delta < frame_time) {
      double sleep_time = frame_time - delta;
      std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
    }
    start_time = glfwGetTime();

    
    quit = should_quit(window);
    //glfwSwapInterval(1);
    glfwGetWindowSize(window, &width, &height);
    mouse_pos = get_mouse_pos(window);

  
    if (is_key_pressed(window, GLFW_KEY_LEFT)) {
      translate.x -= 0.05f;
      std::cout << "translated: " << glm::to_string(translate) << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_RIGHT)) {
      translate.x += 0.05f;
      std::cout << "translated: " << glm::to_string(translate) << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_UP)) {
      translate.y += 0.05f;
      std::cout << "translated: " << glm::to_string(translate) << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_DOWN)) {
      translate.y -= 0.05f;
      std::cout << "translated: " << glm::to_string(translate) << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_S)) {
      translate.z -= 0.05f;
      std::cout << "translated: " << glm::to_string(translate) << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_W)) {
      translate.z += 0.05f;
      std::cout << "translated: " << glm::to_string(translate) << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_A)) {
      angle = ((int)angle + 5) % 360;
      std::cout << "rotated: " << angle << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_D)) {
      angle = ((int)angle - 5) % 360;
      std::cout << "rotated: " << angle << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_V)) {
      if (start_time - key_time > key_threshold) {
	key_time = start_time;
	mesh_set.mode = (VISUALIZATION_MODE)(((uint32_t)mesh_set.mode + 1) % (WIREFRAME + 1));
      }
    }

    glPolygonMode(GL_FRONT_AND_BACK, mesh_set.mode == FILL_POLYGON ? GL_FILL : GL_LINE);

    if (is_mouse_button_pressed(window, GLFW_MOUSE_BUTTON_LEFT)) {
      if (start_time - click_time > threshold) {
	click_time = start_time;
	//glClearColor(0.99, 0.3, 0.3, 1.0);

	total_click++;
      }

    } else if (is_mouse_button_pressed(window, GLFW_MOUSE_BUTTON_RIGHT)) {
      if (start_time - click_time > threshold) {
	click_time = start_time;
	total_click++;
      }
    }
    else {
      // draw
      {
	//glClearColor(1.0 * (mouse_pos.x/1000.0f), 1.0 * (mouse_pos.y/1000.0f), 1.0 * (((mouse_pos.x + mouse_pos.y) / 2) / 1000.0f), 1.0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);

    mesh_set.translate = translate;
    mesh_set.scale = scale;
    mesh_set.angle = angle;
    //mesh_set.axis = axis;
    //glBindVertexArray(VAO);
    draw(VAO, program, mesh_set);

    show_global_info();
    show_global_settings(&mesh_set);
    show_model_matrix(&mesh_set);
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwDestroyCursor(cursor);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

int main(int argc, char **argv) {

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
      std::cout << "(v): troca o modo de visualização de fill para line (wireframe)." << std::endl;
      std::cout << "(w): faz deslocamento positivo em z." << std::endl;
      std::cout << "(s): faz deslocamento negativo em z." << std::endl;
      std::cout << "(seta para baixo): faz deslocamento negativo em y." << std::endl;
      std::cout << "(seta para cima): faz deslocamento postivo em y." << std::endl;
      std::cout << "(seta para esquerda): faz deslocamento negativo em x." << std::endl;
      std::cout << "(seta para direita): faz deslocamento positivo em x." << std::endl << std::endl;
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

    return 0;
  }

  auto result = ObjLoader::load_obj(argc, argv);
  if (result.shapes.empty()) {
    std::cerr << "precisa de 1 shape" << std::endl;
    exit(1);
  }

  std::vector<Vertex> verts;
  std::vector<uint32_t> idx;
  
  const auto &shape = result.shapes[0];
  const auto &indices = shape.mesh.indices;
  const auto &p = result.attributes.positions;

  for (const auto &index : indices) {
    idx.push_back((uint32_t)index.position_index);
  }

  std::vector<Position> positions;
  for (uint32_t i = 0; i < p.size(); i += 3){
    Position pos = (Position){
      .x = p[i + 0],
      .y = p[i + 1],
      .z = p[i + 2],
      .w = 1.0f
    };
    Color color = (Color){
      .r = 1.0f,
      .g = 0.5f,
      .b = 1.0f,
      .a = 1.0f,
    };
    verts.push_back(
      (Vertex){
	.position = pos,
	.color = color,
      }
    );
  }
  
  MeshSettings mesh_set = (MeshSettings){
    .obj_file = argv[1],
    .mode = FILL_POLYGON,
    .vertices = verts,
    .t_verts = verts.size(),
    .indices = idx,
    .t_idx = idx.size(),
    .translate = glm::vec3(0.0f),
    .scale = glm::vec3(0.5f),
    .angle = 0.0f,
    .axis = glm::vec3(1.0f)
  };

  std::cout << "mesh total triangles: " << mesh_set.t_idx / 3 << std::endl;
  std::cout << "mesh total idx: " << mesh_set.t_idx << std::endl;
  std::cout << "mesh total verts: " << mesh_set.t_verts << std::endl;
  std::cout << "mesh verts size: " << sizeof(Vertex) * mesh_set.t_verts << std::endl;
  std::cout << "mesh idx size: " << sizeof(mesh_set.indices[0]) * mesh_set.t_idx << std::endl;
 

  if (!glfwInit()) {
    std::cerr << "Could not initialize glfw!" << std::endl;
    std::cerr << "error: " << strerror(errno) << std::endl;
    exit(1);
  }

  glfwInitHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwInitHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwInitHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

  const char *title = "trackball - pizza";

  GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, title, nullptr, nullptr);
  
  if (window == nullptr) {
    glfwTerminate();
    std::cerr << "Could not create glfw window!" << std::endl;
    std::cerr << "error: " << strerror(errno) << std::endl;
    exit(1);
  }

  glfwMakeContextCurrent(window);
  
  uint32_t err = glewInit();
  if (GLEW_OK != err) {
    std::cerr << "GLEW initialization error!" << std::endl;
    std::cerr << "error: " << strerror(errno);
  }

  std::cout << glGetString(GL_VERSION) << std::endl;
  std::cout << glGetString(GL_RENDERER) << std::endl;
  std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  loop(window, mesh_set);

  glfwTerminate();
  return 0;
}
