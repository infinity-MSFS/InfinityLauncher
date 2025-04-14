
#include "Background.hpp"

#include <iostream>
#include <ranges>
#include <unordered_map>

namespace Infinity {


  const std::unordered_map<std::string, ImVec2> defaultCirclePositions = {{"Circle1", ImVec2(100, 100)},
                                                                          {"Circle2", ImVec2(200, 200)},
                                                                          {"Circle3", ImVec2(300, 300)},
                                                                          {"Circle4", ImVec2(400, 400)},
                                                                          {"Circle5", ImVec2(200, 50)}};


  Background::Background()
      : m_primary_color(HomePagePrimary)
      , m_secondary_color(HomePageSecondary)
      , m_circle_color1({18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f})
      , m_circle_color2({221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f})
      , m_circle_color3({100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f})
      , m_circle_color4({200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f})
      , m_circle_color5({180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f})
      , m_window_size({0.0f, 0.0f})
      , m_window_pos({0.0f, 0.0f})
      , m_home_page(true)
      , m_dot_opacity(0.3f)
      , m_target_dot_opacity(0.3f)

  {
    if (s_circle_pos.empty()) {
      s_circle_pos.resize(5);
    }
  }

  void Background::RenderBackground() {
    m_window_pos = ImGui::GetWindowPos();
    m_window_size = ImGui::GetWindowSize();

    UpdateDotOpacity();
    RenderBackgroundBaseLayer();
    RenderBackgroundGradientLayer();
    RenderBackgroundDotsLayer();
  }


  void Background::RenderBackgroundBaseLayer() {
    ImGui::GetWindowDrawList()->AddRectFilled(
        m_window_pos, ImVec2(m_window_pos.x + m_window_size.x, m_window_pos.y + m_window_size.y),
        ImColor(0.0f, 0.0f, 0.0f, 1.0f));

    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
        m_window_pos, ImVec2(m_window_pos.x + m_window_size.x, m_window_pos.y + m_window_size.y),
        ImColor(m_primary_color), ImColor(m_primary_color), ImColor(m_secondary_color), ImColor(m_secondary_color));
  }

  void Background::RenderGradientCircle(const ImVec2 center, const float radius, const float maxOpacity,
                                        const ImU32 color) {
    constexpr int layers = 80;

    const ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(color);

    for (int i = 0; i < layers; i++) {
      constexpr int segments = 150;

      const float layerRadius = radius * (layers - static_cast<float>(i)) / layers;
      float alpha = colorVec.w * (static_cast<float>(i) + 1.0f) / layers;

      if (alpha > maxOpacity) {
        alpha = maxOpacity;
      }

      const ImU32 layerColor = ImGui::ColorConvertFloat4ToU32(ImVec4(colorVec.x, colorVec.y, colorVec.z, alpha));

      ImGui::GetWindowDrawList()->AddCircleFilled(center, layerRadius, layerColor, segments);
    }
  }

  void Background::TrySetDefaultPositions() {
    if (s_circle_pos.size() == 5) {
      int i = 0;
      for (const auto& val: defaultCirclePositions | std::views::values) {
        InitializeCirclePosition(i++, val);
        if (i >= 5) break;
      }
    }
  }

  void Background::SetDotOpacity(const float opacity) { m_target_dot_opacity = opacity; }

  void Background::UpdateDotOpacity() {
    constexpr float transitionSpeed = 0.003f;

    if (m_dot_opacity < m_target_dot_opacity) {
      m_dot_opacity += transitionSpeed;
      if (m_dot_opacity > m_target_dot_opacity) {
        m_dot_opacity = m_target_dot_opacity;
      }
    } else if (m_dot_opacity > m_target_dot_opacity) {
      m_dot_opacity -= transitionSpeed;
      if (m_dot_opacity < m_target_dot_opacity) {
        m_dot_opacity = m_target_dot_opacity;
      }
    }
  }


  void Background::CreateDotTexture() {
    constexpr int texture_size = 15;
    unsigned char texture_data[texture_size * texture_size * 4] = {0};

    constexpr float x_center = texture_size / 2.0f;
    constexpr float y_center = texture_size / 2.0f;
    constexpr float radius = 1.0f;
    constexpr float aa_width = 1.0f;

    for (int y = 0; y < texture_size; ++y) {
      for (int x = 0; x < texture_size; ++x) {
        const float dx = static_cast<float>(x) - x_center + 0.5f;
        const float dy = static_cast<float>(y) - y_center + 0.5f;
        float dist = std::sqrt(dx * dx + dy * dy);

        float alpha = 0.0f;

        if (dist <= radius) {
          alpha = 1.0f;
        } else if (dist <= radius + aa_width) {
          float t = (dist - radius) / aa_width;
          alpha = 1.0f - t;
        }

        if (alpha > 0.0f) {
          int index = (y * texture_size + x) * 4;
          texture_data[index + 0] = 255;
          texture_data[index + 1] = 255;
          texture_data[index + 2] = 255;
          texture_data[index + 3] = static_cast<unsigned char>(alpha * m_dot_opacity * 255.0f);
        }
      }
    }

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_size, texture_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);

    m_dot_texture = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture_id));
  }


  void Background::RenderBackgroundDotsLayer() {
    if (!m_dot_texture) {
      CreateDotTexture();
    }

    const auto offsetPosition = ImVec2(m_window_pos.x + 10, m_window_pos.y + 10);
    const float windowWidth = ImGui::GetWindowWidth();
    const float windowHeight = ImGui::GetWindowHeight();
    const auto color = ImColor(1.0f, 1.0f, 1.0f, m_dot_opacity);

    const ImVec2 uv_min(10.0f / 15.0f, 10.0f / 15.0f);
    const ImVec2 uv_max(uv_min.x + windowWidth / 15.0f, uv_min.y + windowHeight / 15.0f);

    ImGui::GetWindowDrawList()->AddImage(m_dot_texture, offsetPosition,
                                         ImVec2(offsetPosition.x + windowWidth, offsetPosition.y + windowHeight),
                                         uv_min, uv_max, color);
  }


  void Background::RenderBackgroundGradientLayer() {
    static float circle1angle = 0.0f;
    static float circle2angle = 0.0f;
    static float circle3angle = 0.0f;
    static float circle4angle = 0.0f;
    static float circle5angle = 0.0f;

    TrySetDefaultPositions();


    RenderGradientCircle(ImVec2(m_window_pos.x + s_circle_pos[0].x, m_window_pos.y + s_circle_pos[0].y), 600.0f, 0.01f,
                         ImColor(m_circle_color1));
    RenderGradientCircle(ImVec2(m_window_pos.x + s_circle_pos[1].x, m_window_pos.y + s_circle_pos[1].y), 400.0f, 0.01f,
                         ImColor(m_circle_color2));
    RenderGradientCircle(ImVec2(m_window_pos.x + s_circle_pos[2].x, m_window_pos.y + s_circle_pos[2].y), 600.0f, 0.01f,
                         ImColor(m_circle_color3));
    RenderGradientCircle(ImVec2(m_window_pos.x + s_circle_pos[3].x, m_window_pos.y + s_circle_pos[3].y), 400.0f, 0.01f,
                         ImColor(m_circle_color4));
    RenderGradientCircle(ImVec2(m_window_pos.x + s_circle_pos[4].x, m_window_pos.y + s_circle_pos[4].y), 600.0f, 0.01f,
                         ImColor(m_circle_color5));


    if (circle1angle - 0.05f < 0.0f)
      circle1angle = 360.0f;
    else
      circle1angle -= 0.05f;
    if (circle2angle + 0.05f > 360)
      circle2angle = 0;
    else
      circle2angle += 0.05f;
    if (circle3angle - 0.05f < 0.0f)
      circle3angle = 360.0f;
    else
      circle3angle -= 0.05f;
    if (circle4angle + 0.05f > 360)
      circle4angle = 0;
    else
      circle4angle += 0.05f;
    if (circle5angle - 0.05f < 360)
      circle5angle = 0;
    else
      circle5angle -= 0.05f;

    const auto screen_center = ImVec2(ImGui::GetWindowSize().x / 2.0f, ImGui::GetWindowSize().y / 2.0f);

    s_circle_pos[0] = GetCircleCoords(600.0f, circle1angle, ImVec2(screen_center.x - 100.0f, screen_center.y + 200.0f));
    s_circle_pos[1] = GetCircleCoords(500.0f, circle2angle, ImVec2(screen_center.x + 100.0f, screen_center.y + 20.0f));
    s_circle_pos[2] = GetCircleCoords(155.0f, circle3angle, ImVec2(screen_center.x - 200.0f, screen_center.y - 100.0f));
    s_circle_pos[3] = GetCircleCoords(409.0f, circle4angle, ImVec2(screen_center.x + 0.0f, screen_center.y + 200.0f));
    s_circle_pos[4] = GetCircleCoords(781.0f, circle5angle, ImVec2(screen_center.x - 100.0f, screen_center.y + 0.0f));
  }


}  // namespace Infinity
