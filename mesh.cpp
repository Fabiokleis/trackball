#include "mesh.hpp"
#include "./dependencies/imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void show_global_info(MeshSettings *mesh_set) {
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
      ImGui::Text("width %.1f height %.1f", mesh_set->resolution.x, mesh_set->resolution.y);
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

void show_global_settings(MeshSettings *mesh_set) {
  ImGuiIO& io = ImGui::GetIO(); (void) io;
  static int menu_item = 0;
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav;
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  
  if (ImGui::Begin("mesh", nullptr, window_flags)) {
    ImGui::Text("malha: %s", mesh_set->obj_file);
    ImGui::Text("textura: %s", mesh_set->tex_file);
    ImGui::Text("modo de visualização (v): %s", mesh_set->mode == FILL_POLYGON ? "fill polygon" : "polygon wireframe");
    ImGui::Text("luz (1): %s", mesh_set->light ? "ligada" : "desligada");
    switch (mesh_set->tex_mode) {
    case ORTHO:
      ImGui::Text("modo de textura: %s", "ortografica");
      break;
    case CIL:
      ImGui::Text("modo de textura: %s", "cilíndrica");
      break;
    case SPH:
      ImGui::Text("modo de textura: %s", "esférica");
      break;
    case NO_TEX:
    default:
      ImGui::Text("modo de textura: %s", "sem textura");
    }

    ImGui::Separator();
    ImGui::Text("vertices: %lu", mesh_set->t_verts);
    ImGui::Text("indices: %lu", mesh_set->t_index);
    ImGui::Text("triangulos: %lu", mesh_set->t_index / 3);

    if (ImGui::BeginPopupContextWindow()) {
      if (ImGui::MenuItem("trocar modo de visualização (v)", NULL, menu_item == 1)) {
	menu_item = 0;
	mesh_set->mode = (VISUALIZATION_MODE)(((uint32_t)mesh_set->mode + 1) % (WIREFRAME + 1));
      } else if (ImGui::MenuItem("ligar/desligar luz (1)", NULL, menu_item == 2)) {
	menu_item = 0;
	mesh_set->light = !mesh_set->light;
      } else if (ImGui::MenuItem("habilitar/desabilitar textura ortografica (2)", NULL, menu_item == 3)) {
	menu_item = 0;
	if (mesh_set->tex_mode == NO_TEX) mesh_set->tex_mode = ORTHO;
	else mesh_set->tex_mode = NO_TEX;
      } else if (ImGui::MenuItem("habilitar/desabilitar modo de textura cilíndrica (3)", NULL, menu_item == 4)) {
	menu_item = 0;
	if (mesh_set->tex_mode == NO_TEX) mesh_set->tex_mode = CIL;
	else mesh_set->tex_mode = NO_TEX;
      } else if (ImGui::MenuItem("habilitar/desabilitar modo de textura esférica (4)", NULL, menu_item == 5)) {
	menu_item = 0;
	if (mesh_set->tex_mode == NO_TEX) mesh_set->tex_mode = SPH;
	else mesh_set->tex_mode = NO_TEX;
      }
      
      ImGui::EndPopup();
    }
  }
  ImGui::End();
}

void show_model_matrix(MeshSettings *mesh_set) {
  ImGuiIO& io = ImGui::GetIO(); (void) io;
  //static int menu_item = 0;
  ImGuiWindowFlags window_flags =  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav;
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  
  if (ImGui::Begin("model", nullptr, window_flags)) {
    ImGui::Separator();
    ImGui::InputFloat3("center", &mesh_set->center[0]);
    ImGui::InputFloat3("translacao", &mesh_set->translate[0]);
    ImGui::InputFloat3("scala", &mesh_set->scale[0]);
    ImGui::Separator();
    ImGui::InputFloat("stroke", &mesh_set->stroke);
    ImGui::SliderFloat("scale factor", &mesh_set->scale_factor, 0.01f, 1.0f);
  }
  ImGui::End();
}

void show_lightning(MeshSettings *mesh_set) {
  ImGuiIO& io = ImGui::GetIO(); (void) io;
  ImGuiWindowFlags window_flags =  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing;
  ImGui::Begin("lightning", nullptr, window_flags);
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  ImGui::Separator();
  ImGui::ColorEdit3("background color", &mesh_set->bg_color[0]);
  ImGui::ColorEdit3("lightning color", &mesh_set->light_color[0]);
  ImGui::Separator();
  ImGui::InputFloat3("lightning position", &mesh_set->light_position[0]);
  ImGui::InputFloat3("camera position", &mesh_set->camera_position[0]);
  ImGui::SliderFloat("ka (ambiente)", &mesh_set->ka, 0.0f, 1.0f);
  ImGui::SliderFloat("kd (difusa)", &mesh_set->kd, 0.0f, 1.0f);
  ImGui::SliderFloat("ks (especular)", &mesh_set->ks, 0.0f, 1.0f);
  ImGui::InputFloat("atenuacao de brilho", &mesh_set->ksb);
  ImGui::End();
}

void show_controls(bool *p_open) {
  ImGuiIO& io = ImGui::GetIO(); (void) io;
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  if (!ImGui::Begin("tela de controles", p_open)) {
    ImGui::End();
    return;
  }
  ImGui::BulletText("%s", EXIT_KEY);
  ImGui::BulletText("%s", K_KEY);
  ImGui::BulletText("%s", V_KEY);    
  ImGui::BulletText("%s", W_KEY);
  ImGui::BulletText("%s", S_KEY);
  ImGui::BulletText("%s", UP_KEY);
  ImGui::BulletText("%s", DOWN_KEY);
  ImGui::BulletText("%s", LEFT_KEY);
  ImGui::BulletText("%s", RIGHT_KEY);
  ImGui::BulletText("%s", LIGHT_KEY);
  ImGui::BulletText("%s", TEX_ORTHO);
  ImGui::BulletText("%s", TEX_CIL);
  ImGui::BulletText("%s", TEX_SPH);
  ImGui::End();
}
