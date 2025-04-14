#include "Meteors.hpp"

#include <cmath>
#include <ctime>
#include <imgui.h>
#include <imgui_internal.h>
#include <random>
#include <vector>

namespace Infinity {
  void Meteor::Respawn(const ImVec2 screen_size) {
    std::uniform_real_distribution pos_dist(-500.0f, 500.0f);

    if (RandomRange(0.0f, 1.0f) > 0.5f) {
      position = ImVec2(-100.0f - std::abs(pos_dist(GetRNG())), RandomRange(-100.0f, screen_size.y + 200.0f));
    } else {
      position = ImVec2(RandomRange(-100.0f, screen_size.x + 200.0f), -100.0f - std::abs(pos_dist(GetRNG())));
    }
    speed = RandomRange(0.8f, 1.4f);

    length = 80.0f + RandomRange(0.0f, 120.0f);
    thickness = 0.5f;

    start_color = ImColor(200, 200, 255, 255);
    end_color = ImColor(200, 200, 255, 0);
  }

  void Meteor::Update(const ImVec2 screen_size) {
    position.x += speed;
    position.y += speed;

    if (position.x > screen_size.x + 500 || position.y > screen_size.y + 500) {
      Respawn(screen_size);
    }
  }

  void Meteor::Render() const {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const auto end_pos = ImVec2(position.x - length, position.y - length);

    constexpr int tail_segments = 28;
    for (int i = 1; i < tail_segments; i++) {
      const float alpha = 1.0f - static_cast<float>(i) / tail_segments;
      ImVec2 tail_start = ImLerp(position, end_pos, (i - 1.0f) / tail_segments);
      ImVec2 tail_end = ImLerp(position, end_pos, static_cast<float>(i) / tail_segments);

      draw_list->AddLine(tail_end, tail_start,
                         ImColor(start_color.Value.x, start_color.Value.y, start_color.Value.z, alpha),
                         thickness * alpha);
    }

    draw_list->AddCircleFilled(position, thickness * 1.5f, ImColor(200, 200, 255, 255));
  }


  void Meteors::Update() {
    ImVec2 screen_size = ImGui::GetWindowSize();
    for (auto& meteor: m_meteors) {
      meteor.Update(screen_size);
    }
  }

  void Meteors::Render() {
    for (const auto& meteor: m_meteors) {
      meteor.Render();
    }
  }


  std::vector<Meteor> meteors;
  int count;

  Meteors::Meteors(const int count)
      : m_count(count) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    meteors.reserve(m_count);
    for (int i = 0; i < m_count; i++) {
      meteors.emplace_back(screen_size);
    }
  };
}  // namespace Infinity
