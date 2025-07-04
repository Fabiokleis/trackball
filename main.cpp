#include <iostream>
#include <cstdint>
#include <string.h>
#include <errno.h>
#include <chrono>
#include <thread>

//#define GLM_ENABLE_EXPERIMENTAL
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/glm.hpp>
//#include <glm/gtx/string_cast.hpp>

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
  layout (location = 1) in vec3 v_normal;
  layout (location = 2) in vec4 v_color;
  uniform mat4 v_model;
  uniform mat4 v_view;
  uniform mat4 v_projection;
  out vec4 color;
  out vec3 normal;
  out vec3 frag_pos;
  out vec3 vpos;

  void main() {
    gl_Position = v_projection * v_view * v_model * v_pos;
    color = v_color;
    normal = mat3(transpose(inverse(v_model))) * v_normal;
    frag_pos = vec3(v_model * v_pos);
    vpos = vec3(v_pos);
  };
)";

// (color * v_color) * v_time
const static char *fragment_shader_source = R"(
  #version 330 core
  in vec4 color;
  in vec3 normal;
  in vec3 frag_pos;
  in vec3 vpos;

  uniform vec2 v_resolution;
  uniform float v_time;
  uniform vec2 v_mouse_pos;

  uniform int v_light;
  uniform vec3 v_camera_position;
  uniform vec3 v_light_position;
  uniform vec3 v_light_color;
  uniform float v_ka;
  uniform float v_kd;
  uniform float v_ks;
  uniform float v_ksb;
  uniform int v_tex_mode;

  uniform sampler2D tex;

  out vec4 FragColor;

  #define PI 3.1415926535897932384626433832795
  #define NO_TEX 0
  #define ORTHO 1
  #define CIL 2
  #define SPH 3


  vec4 phong() {
     vec3 ambient = v_ka * v_light_color;

     vec3 l = normalize(v_light_position - frag_pos);
     float diff = max(dot(normal, l), 0.0);
     vec3 diffuse = v_kd * diff * v_light_color;

     vec3 v = normalize(v_camera_position - frag_pos);
     vec3 r = reflect(-l, normal);
     float spec = pow(max(dot(v, r), 0.0), v_ksb);
     vec3 specular = v_ks * spec * v_light_color;

     vec4 out_light = vec4(ambient + diffuse + specular, 1.0f);
     return out_light;
  }

  vec2 ortho(vec3 pos) {
    return pos.xy + 0.5f; // -1 .. 1
  }

  vec2 cil(vec3 pos) {
    float u = (PI + atan(pos.z, pos.x)) / (2 * PI);
    float v = 0.5f + (0.5f * pos.y);
    return vec2(u, v);
  }

  vec2 sph(vec3 pos) {
    float u = (PI + atan(pos.z, pos.x)) / (2 * PI);
    float v = (acos(pos.y / (length(pos.xyz)))) / PI;
    return vec2(u, v);
  }


  void main()
  {
     vec4 light = v_light == 1 ? phong() : vec4(1.0f);
     vec4 color = vec4(0.5f + 0.5 * cos(v_time + color.xyz + vec3(0.0f, 2.0f, 4.0f)), 1.0f);

     vec2 uv = vpos.xy;
     switch (v_tex_mode) {
     case ORTHO:
       uv = ortho(vpos.xyz);
       break;
     case CIL:
       uv = cil(vpos.xyz);
       break;
     case SPH:
       uv = sph(vpos.xyz);
       break;
     case NO_TEX:
     default:
      break;
     }

     vec4 tex_color = texture(tex, uv);
     vec4 out_color = light * (v_tex_mode > 0 ? tex_color : color);

     FragColor = out_color;
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

bool is_key_pressed(GLFWwindow *window, int keycode) {
  int state = glfwGetKey(window, keycode);
  return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool is_mouse_button_pressed(GLFWwindow *window, int button) {
  int state = glfwGetMouseButton(window, button);
  return state == GLFW_PRESS;
}

bool is_mouse_button_released(GLFWwindow *window, int button) {
  int state = glfwGetMouseButton(window, button);
  return state == GLFW_RELEASE;
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

glm::vec2 get_mouse_pos(GLFWwindow *window) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return glm::vec2(xpos, ypos);
}
// mouse offset 1 -1
glm::vec3 mouse_to_gl_point(float x, float y) {
  return glm::vec3((2.0f * x) / WIDTH - 1.0f, 1.0f - (2.0f * y) / HEIGHT, 0.0f);
}

glm::vec3 mouse_to_trackball(glm::vec2 pos) {
  glm::vec2 gl_pos = mouse_to_gl_point(pos.x, pos.y);
  // x2 + y2 + z2 = 1  -> z2 = 1 − x2 −y2
  float z1 = 1.0f - gl_pos.x * gl_pos.x - gl_pos.y * gl_pos.y;    
  float z = z1 > 0 ? z1 : 0;
  return glm::normalize(glm::vec3(gl_pos.x, gl_pos.y, z));
}

glm::quat rotation_calc(glm::vec2 l_mouse_pos, glm::vec2 c_mouse_pos) {
  glm::vec3 n_l_pos = mouse_to_trackball(l_mouse_pos);
  glm::vec3 n_c_pos = mouse_to_trackball(c_mouse_pos);
  
  glm::vec3 axis = glm::cross(n_l_pos, n_c_pos);
  float dot = glm::dot(n_l_pos, n_c_pos);
  float angle = acos(glm::clamp(dot, -1.0f, 1.0f));
  
  if (glm::length(axis) < 0.00001f || angle < 0.00001f) {
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  }

  return glm::angleAxis(angle, glm::normalize(axis));
}

void draw(uint32_t VAO, uint32_t program, MeshSettings* mesh_set, glm::vec2 c_mouse_pos) {
  float time = (float)glfwGetTime();
  glm::mat4 view = glm::mat4(1.0f);
  view = glm::lookAt(mesh_set->camera_position, 
		     glm::vec3(0.0f, 0.0f, 0.0f), 
		     glm::vec3(0.0f, 1.0f, 0.0f));
  
  glm::mat4 projection = glm::mat4(1.0f);
  glm::mat4 model = glm::mat4(1.0f);

  /* T * R * S * T <- */
  model = glm::translate(model, mesh_set->translate);

  if (mesh_set->rotating) {
    glm::quat delta = rotation_calc(mesh_set->mouse_pos, c_mouse_pos);
    mesh_set->rotation = glm::normalize(delta * mesh_set->rotation);
    mesh_set->mouse_pos = c_mouse_pos;
  }
  
  model = model * glm::mat4_cast(mesh_set->rotation);
  model = glm::scale(model, mesh_set->scale);
  //model = glm::translate(model, -mesh_set->center); nao precisa mais

  projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

  int v_resolution = glGetUniformLocation(program, "v_resolution");
  int v_model = glGetUniformLocation(program, "v_model");
  int v_view = glGetUniformLocation(program, "v_view");
  int v_projection = glGetUniformLocation(program, "v_projection");
  int v_time = glGetUniformLocation(program, "v_time");
  int v_light = glGetUniformLocation(program, "v_light");
  int v_camera_position = glGetUniformLocation(program, "v_camera_position");
  int v_light_position = glGetUniformLocation(program, "v_light_position");
  int v_light_color = glGetUniformLocation(program, "v_light_color");
  int v_ka = glGetUniformLocation(program, "v_ka");
  int v_kd = glGetUniformLocation(program, "v_kd");
  int v_ks = glGetUniformLocation(program, "v_ks");
  int v_ksb = glGetUniformLocation(program, "v_ksb");
  int v_tex_mode = glGetUniformLocation(program, "v_tex_mode");

  glUniformMatrix4fv(v_model, 1, GL_FALSE, &model[0][0]);
  glUniformMatrix4fv(v_view, 1, GL_FALSE, &view[0][0]);
  glUniformMatrix4fv(v_projection, 1, GL_FALSE, &projection[0][0]);

  glUniform2f(v_resolution, mesh_set->resolution.x, mesh_set->resolution.y);
  glUniform1f(v_time, time);
  glUniform1i(v_light, (int)mesh_set->light);
  glUniform1i(v_tex_mode, (int)mesh_set->tex_mode);
  glUniform3f(v_camera_position, mesh_set->camera_position[0], mesh_set->camera_position[1], mesh_set->camera_position[2]);
  glUniform3f(v_light_position, mesh_set->light_position[0], mesh_set->light_position[1], mesh_set->light_position[2]);
  glUniform3f(v_light_color, mesh_set->light_color[0], mesh_set->light_color[1], mesh_set->light_color[2]);
  glUniform1f(v_ka, mesh_set->ka);
  glUniform1f(v_kd, mesh_set->kd);
  glUniform1f(v_ks, mesh_set->ks);
  glUniform1f(v_ksb, mesh_set->ksb);
  glLineWidth(mesh_set->stroke);

  glBindVertexArray(VAO);
  //glDrawArrays(GL_TRIANGLES, 0, mesh_set->t_verts);
  glDrawElements(GL_TRIANGLES, mesh_set->t_index, GL_UNSIGNED_INT, 0);
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  //glUniform4f(v_bord_color, 0.1f, 0.0f, 0.0f, 1.0f);  
  //glDrawArrays(GL_TRIANGLES, 0, mesh_set->t_verts);
}

void loop(GLFWwindow *window) {

  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NavEnableKeyboard;
  
  ImGui::StyleColorsClassic();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init((char *)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  
  int width = 0;
  int height = 0;

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

  uint32_t VAO, VBO, EBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);
  
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, mesh_set->t_verts * sizeof(Vertex), &mesh_set->vertices[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_set->indices.size() * sizeof(uint32_t), &mesh_set->indices[0], GL_STATIC_DRAW);
  
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
  glEnableVertexAttribArray(0); // location 0

  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
  glEnableVertexAttribArray(1); // location 1

  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
  glEnableVertexAttribArray(2); // location 1

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0); 

  glEnable(GL_DEPTH_TEST);

  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POLYGON_SMOOTH);
  glEnable(GL_MULTISAMPLE);

  stbi_set_flip_vertically_on_load(1);
  unsigned int tex;
  
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int i_width, i_height, nr_channels;

  if (mesh_set->tex_file != nullptr) {
    unsigned char *data = stbi_load(mesh_set->tex_file, &width, &height, &nr_channels, 0);
    if (data) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
    } else {
      std::cout << "ERROR: Failed to load texture" << std::endl;
      exit(1);
    }
  
    stbi_image_free(data); // loaded
  }
  (void)i_width, (void)i_height;

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
  
  while (!quit) {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    show_global_info(mesh_set);
    show_global_settings(mesh_set);
    show_model_matrix(mesh_set);
    show_lightning(mesh_set);
    
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

    if (start_time - key_time > key_threshold && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !ImGui::IsAnyItemActive()) {
      if (is_key_pressed(window, GLFW_KEY_1)) {
	mesh_set->light = !mesh_set->light;
	key_time = start_time;
      } else if (is_key_pressed(window, GLFW_KEY_2)) {
	if (mesh_set->tex_mode == ORTHO) mesh_set->tex_mode = NO_TEX;
	else mesh_set->tex_mode = ORTHO;
	key_time = start_time;
      } else if (is_key_pressed(window, GLFW_KEY_3)) {
	if (mesh_set->tex_mode == CIL) mesh_set->tex_mode = NO_TEX;
	else mesh_set->tex_mode = CIL;
	key_time = start_time;
      } else if (is_key_pressed(window, GLFW_KEY_4)) {
	if (mesh_set->tex_mode == SPH) mesh_set->tex_mode = NO_TEX;
	else mesh_set->tex_mode = SPH;
	key_time = start_time;
      }
    }
    
    glPolygonMode(GL_FRONT_AND_BACK, mesh_set->mode == FILL_POLYGON ? GL_FILL : GL_LINE);

    if (is_mouse_button_pressed(window, GLFW_MOUSE_BUTTON_LEFT)) {
      if (start_time - click_time > threshold) {
        click_time = start_time;
	 //glClearColor(0.99, 0.3, 0.3, 1.0);
        if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !ImGui::IsAnyItemActive()) {
	  mesh_set->rotating = true;
	  mesh_set->mouse_pos = get_mouse_pos(window);
	  //std::cout << glm::to_string(mesh_set->mouse_pos) << std::endl;
	  total_click++;
        }
      }

    } else if (is_mouse_button_released(window, GLFW_MOUSE_BUTTON_LEFT)) {
      mesh_set->rotating = false;
    } else if (is_mouse_button_pressed(window, GLFW_MOUSE_BUTTON_RIGHT)) {
      if (start_time - click_time > threshold) {
	click_time = start_time;
	total_click++;
      }
    } else {
      // draw
      {
	//glClearColor(1.0 * (mouse_pos.x/1000.0f), 1.0 * (mouse_pos.y/1000.0f), 1.0 * (((mouse_pos.x + mouse_pos.y) / 2) / 1000.0f), 1.0);
      }

    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(mesh_set->bg_color[0], mesh_set->bg_color[1], mesh_set->bg_color[2], 1.0f);
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    draw(VAO, program, mesh_set, get_mouse_pos(window));

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
  glfwWindowHint(GLFW_SAMPLES, 4);

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
