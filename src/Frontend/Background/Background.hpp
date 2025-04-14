
#pragma once


#include <cmath>

#include "GL/glew.h"
//
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "GL/gl.h"
#include "imgui.h"
#include "vector"


namespace Infinity {

  const ImVec4 HomePagePrimary(18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.11f);
  const ImVec4 HomePageSecondary(221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.11f);

  static std::vector<ImVec2> s_circle_pos;


  class Background {
public:
    static Background *GetInstance() {
      static Background instance;
      return &instance;
    }


    void SetDotOpacity(float opacity);

    void RenderBackground();

    void UpdateColorScheme() {
      auto &interpolator = ColorInterpolation::GetInstance();

      constexpr auto easing_types = Easing::EasingTypes::EaseInOutCubic;

      const auto colors = interpolator.GetCurrentGradientColors(easing_types);
      m_primary_color = std::get<0>(colors);
      m_secondary_color = std::get<1>(colors);
      m_circle_color1 = std::get<2>(colors);
      m_circle_color2 = std::get<3>(colors);
      m_circle_color3 = std::get<4>(colors);
      m_circle_color4 = std::get<5>(colors);
      m_circle_color5 = std::get<6>(colors);
    }

    void SetHomePage(const bool state) {
      if (state) {
        auto &interpolator = ColorInterpolation::GetInstance();
        interpolator.ChangeGradientColors(
            HomePagePrimary, HomePageSecondary, {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.01f},
            {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.01f}, {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.01f},
            {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.01f},
            {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.01f}, 1.0f);
        m_home_page = true;
      } else {
        m_home_page = state;
      }
    }


private:
    void UpdateDotOpacity();

    void RenderBackgroundDotsLayer();

    void RenderBackgroundGradientLayer();


    void RenderBackgroundBaseLayer();

    void RenderGradientCircle(ImVec2 center, float radius, float maxOpacity, ImU32 color);

    void InitializeCirclePosition(const int index, const ImVec2 position) {
      if (index >= 0 && index < s_circle_pos.size() && s_circle_pos[index].x == 0 && s_circle_pos[index].y == 0) {
        s_circle_pos[index] = position;
      }
    }

    void TrySetDefaultPositions();

    static ImVec2 GetCircleCoords(const float radius, const float theta, const ImVec2 center) {
      const float radians = (3.141592f / 180.0f) * theta;
      float x = center.x + radius * cosf(radians);
      float y = center.y + radius * sinf(radians);

      return {x, y};
    }

    Background();

    void CreateDotTexture();

private:
    ImVec2 m_window_pos;
    ImVec2 m_window_size;
    ImVec4 m_primary_color;
    ImVec4 m_secondary_color;
    ImVec4 m_circle_color1;
    ImVec4 m_circle_color2;
    ImVec4 m_circle_color3;
    ImVec4 m_circle_color4;
    ImVec4 m_circle_color5;
    bool m_home_page;
    float m_dot_opacity;
    float m_target_dot_opacity;
    ImTextureID m_dot_texture = nullptr;
  };


}  // namespace Infinity
