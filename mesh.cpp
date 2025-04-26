#include "mesh.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


void show_global_info() {
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

void show_global_settings(MeshSettings *mesh_set) {
  ImGuiIO& io = ImGui::GetIO(); (void) io;
  static int menu_item = 0;
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav;
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  
  if (ImGui::Begin("mesh", nullptr, window_flags)) {
    ImGui::Text("malha: %s", mesh_set->obj_file);
    ImGui::Text("modo (v): %s", mesh_set->mode == FILL_POLYGON ? "fill polygon" : "polygon wireframe");
    ImGui::Separator();
    ImGui::Text("vertices: %lu", mesh_set->t_verts);
    ImGui::Text("triangulos: %lu", mesh_set->t_verts / 3);

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

void show_model_matrix(MeshSettings *mesh_set) {
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
