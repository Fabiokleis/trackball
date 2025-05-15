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


#include "mesh.hpp"
#include "obj.hpp"

MeshSettings *mesh_set;

const static char *vertex_shader_source = R"(
  #version 330 core
  layout (location = 0) in vec4 v_pos;
  layout (location = 1) in vec4 v_color;
  uniform mat4 v_model;
  uniform mat4 v_view;
  uniform mat4 v_projection;
  out vec4 color;
  out vec2 frag_coord;
  out float v_z;
  void main() {
     gl_Position = v_projection * v_view * v_model * v_pos;
     color = v_color;
     frag_coord = v_pos.xy;
     v_z = v_pos.z;
  };
)";

// (color * v_color) * v_time
const static char *fragment_shader_source = R"(
  #version 330 core
  in vec4 color;
  in vec2 frag_coord;
  in float v_z;
  uniform vec2 v_resolution;
  uniform float v_time;
  uniform vec4 v_bord_color;
  uniform vec4 v_mix_color;
  uniform float v_blend;
  uniform vec2 v_mouse_pos;

  out vec4 FragColor;

  void main()
  {
     vec4 c = vec4(0.5f + 0.5 * cos(v_time + color.xyz + vec3(0.0f, 2.0f, 4.0f)), 1.0f);
     //vec2 uv = (frag_coord * 2.0 - frag_coord.xy) / frag_coord.y;
     if (v_bord_color.w > 0.0f) {

       //float d = length(uv);
       //vec3 col = 0.5 + 0.5 * cos(v_time + uv.xyx + vec3(1.0f, 2.0f, 3.0f));
       //d = sin(d * 8.0f + v_time) / 8.0f;
       //d = abs(d);
       //d = 0.01f / d;
   
       //FragColor = vec4(col, 1.0f);
       FragColor = mix(color, c, v_blend);
     } else {
       //vec4 c1 = mix(c, vec4(c.x * sin(v_time), c.y, c.z * cos(v_time), 1.0f), 0.5f);
       vec3 c3 = color.xyz;
       vec3 cm3 = v_mix_color.xyz;

       FragColor = vec4(mix(c3, cm3, v_blend), 1.0f);
     }
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
  mesh_set->scale = mesh_set->scale + glm::vec3((yoffset * mesh_set->scale_factor));
  //std::cout << glm::to_string(mesh_set->scale) << std::endl;
}

Vec2 get_mouse_pos(GLFWwindow *window) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return Vec2{ .x = xpos, .y = ypos };
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

  /* T * R * S * T <- */
  model = glm::translate(model, mesh_set.translate);
  model = glm::rotate(model, glm::radians(mesh_set.angle.y), glm::vec3(1.f, 0.0f, 0.0f));
  model = glm::rotate(model, glm::radians(mesh_set.angle.x), glm::vec3(0.f, 1.0f, 0.0f));
  model = glm::scale(model, mesh_set.scale);
  model = glm::translate(model, -mesh_set.center);

  projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f); // glm::ortho(0.0f, (float)WIDTH, 0.0f, (float)HEIGHT, 0.1f, 100.0f);

  int v_resolution = glGetUniformLocation(program, "v_resolution");
  int v_model = glGetUniformLocation(program, "v_model");
  int v_view = glGetUniformLocation(program, "v_view");
  int v_projection = glGetUniformLocation(program, "v_projection");
  int v_time = glGetUniformLocation(program, "v_time");
  int v_bord_color = glGetUniformLocation(program, "v_bord_color");
  int v_mix_color = glGetUniformLocation(program, "v_mix_color");
  int v_blend = glGetUniformLocation(program, "v_blend");
  int v_mouse_pos = glGetUniformLocation(program, "v_mouse_pos");

  glUniformMatrix4fv(v_model, 1, GL_FALSE, &model[0][0]);
  glUniformMatrix4fv(v_view, 1, GL_FALSE, &view[0][0]);
  glUniformMatrix4fv(v_projection, 1, GL_FALSE, &projection[0][0]);

  glUniform2f(v_resolution, mesh_set.resolution.x, mesh_set.resolution.y);
  glUniform1f(v_time, time);
  glUniform4f(v_bord_color, -1.0f, -1.0f, -1.0f, -1.0f);
  glUniform4f(v_mix_color, mesh_set.color[0], mesh_set.color[1], mesh_set.color[2], 1.0f);
  glUniform1f(v_blend, mesh_set.blend);
  glUniform2f(v_mouse_pos, mesh_set.angle.x, mesh_set.angle.y);
  glLineWidth(mesh_set.stroke);

  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, mesh_set.t_verts);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glUniform4f(v_bord_color, 0.1f, 0.0f, 0.0f, 1.0f);  
  glDrawArrays(GL_TRIANGLES, 0, mesh_set.t_verts);
}

void loop(GLFWwindow *window) {

  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  
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

  uint32_t VAO, VBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, mesh_set->t_verts * sizeof(Vertex), &mesh_set->vertices[0], GL_STATIC_DRAW);
  
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
  glEnableVertexAttribArray(0); // location 0

  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
  glEnableVertexAttribArray(1); // location 1

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0); 

  glEnable(GL_DEPTH_TEST);

  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POLYGON_SMOOTH);


  float start_time = glfwGetTime();
  float delta = 0.0f;
  float total_time = 0.0f;

  float frame_time = 1.0f / 30.0f;
  
  float click_time = 0.0f;
  float threshold = 0.2f; // mouse

  float key_time = 0.0f;
  float key_threshold = 0.2f;
  uint32_t total_click = 0;
  bool help = false;

  float vel = 0.0f;
  
  while (!quit) {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    show_global_info(mesh_set);
    show_global_settings(mesh_set);
    show_model_matrix(mesh_set);
    show_color_blend(mesh_set);
    
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
      mesh_set->translate.x -= 0.05f;
    } else if (is_key_pressed(window, GLFW_KEY_RIGHT)) {
      mesh_set->translate.x += 0.05f;
    } else if (is_key_pressed(window, GLFW_KEY_UP)) {
      mesh_set->translate.y += 0.05f;
    } else if (is_key_pressed(window, GLFW_KEY_DOWN)) {
      mesh_set->translate.y -= 0.05f;
    } else if (is_key_pressed(window, GLFW_KEY_S)) {
      mesh_set->translate.z -= 0.05f;
    } else if (is_key_pressed(window, GLFW_KEY_W)) {
      mesh_set->translate.z += 0.05f;    
    } else if (is_key_pressed(window, GLFW_KEY_V)) {
      if (start_time - key_time > key_threshold) { // debounce
	key_time = start_time;
	mesh_set->mode = (VISUALIZATION_MODE)(((uint32_t)mesh_set->mode + 1) % (WIREFRAME + 1));
      }
    }

    glPolygonMode(GL_FRONT_AND_BACK, mesh_set->mode == FILL_POLYGON ? GL_FILL : GL_LINE);

    if (is_mouse_button_pressed(window, GLFW_MOUSE_BUTTON_LEFT)) {
      //if (start_time - click_time > threshold) {
      //click_time = start_time;
	//glClearColor(0.99, 0.3, 0.3, 1.0);
      if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !ImGui::IsAnyItemActive()) {
	vel += delta;
	Vec2 mpos = get_mouse_pos(window);
	glm::vec3 mouse = mouse_to_gl_point((float)mpos.x, (float)mpos.y);
	mesh_set->angle.x = (float)mpos.x;
	mesh_set->angle.y = (float)mpos.y;	
	
	std::cout << glm::to_string(mouse) << std::endl;
	total_click++;
      }
	//}

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

      //mesh_set->angle.x = 0.0f;
      //mesh_set->angle.y = 0.0f;
    }


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    draw(VAO, program, *mesh_set);


    if (ImGui::IsKeyPressed(ImGuiKey_K)) help = !help;
    if (help) show_controls(&help);
    
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

  MeshSettings m = ObjLoader::load_obj(argc, argv);
  mesh_set = (MeshSettings *) malloc(sizeof(MeshSettings));
  mesh_set = &m;
  
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
  
  loop(window);

  glfwTerminate();
  return 0;
}
