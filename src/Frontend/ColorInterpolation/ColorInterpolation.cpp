#include "ColorInterpolation.hpp"

#include "Util/Easing/Easing.hpp"
#include "Util/Error/Error.hpp"

ColorInterpolation &ColorInterpolation::GetInstance() {
  static ColorInterpolation instance;
  return instance;
}

ColorInterpolation::ColorInterpolation()
    : m_colors{Interpolation(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                             ImVec4(0.0f, 0.0f, 0.0f, 1.0f)),
               Interpolation(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                             ImVec4(0.0f, 0.0f, 0.0f, 1.0f)),
               Interpolation(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                             ImVec4(0.0f, 0.0f, 0.0f, 1.0f)),
               Interpolation(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                             ImVec4(0.0f, 0.0f, 0.0f, 1.0f)),
               Interpolation(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                             ImVec4(0.0f, 0.0f, 0.0f, 1.0f)),
               Interpolation(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                             ImVec4(0.0f, 0.0f, 0.0f, 1.0f)),
               Interpolation(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                             ImVec4(0.0f, 0.0f, 0.0f, 1.0f))}
    , m_duration(0.0f)
    , m_active(false) {}

void ColorInterpolation::ChangeGradientColors(const ImVec4 &GradientStart, const ImVec4 &GradientEnd,
                                              const ImVec4 &CircleColor1, const ImVec4 &CircleColor2,
                                              const ImVec4 &CircleColor3, const ImVec4 &CircleColor4,
                                              const ImVec4 &CircleColor5, float durationSec) {
  std::get<0>(m_colors).startColor = std::get<0>(m_colors).currentColor;
  std::get<0>(m_colors).endColor = GradientStart;

  std::get<1>(m_colors).startColor = std::get<1>(m_colors).currentColor;
  std::get<1>(m_colors).endColor = GradientEnd;

  std::get<2>(m_colors).startColor = std::get<2>(m_colors).currentColor;
  std::get<2>(m_colors).endColor = CircleColor1;

  std::get<3>(m_colors).startColor = std::get<3>(m_colors).currentColor;
  std::get<3>(m_colors).endColor = CircleColor2;

  std::get<4>(m_colors).startColor = std::get<4>(m_colors).currentColor;
  std::get<4>(m_colors).endColor = CircleColor3;

  std::get<5>(m_colors).startColor = std::get<5>(m_colors).currentColor;
  std::get<5>(m_colors).endColor = CircleColor4;

  std::get<6>(m_colors).startColor = std::get<6>(m_colors).currentColor;
  std::get<6>(m_colors).endColor = CircleColor5;

  m_duration = durationSec;
  m_start_time = Clock::now();
  m_active = true;
}

using Infinity::Easing::EasingTypes;

std::tuple<ImVec4, ImVec4, ImVec4, ImVec4, ImVec4, ImVec4, ImVec4> ColorInterpolation::GetCurrentGradientColors(
    EasingTypes easing_type) {
  if (m_active) {
    float elapsedTime = std::chrono::duration<float>(Clock::now() - m_start_time).count();
    float raw_t = std::clamp(elapsedTime / m_duration, 0.0f, 1.0f);

    const float t = GetEasing(easing_type, raw_t);

    Interpolation &interpolation0 = std::get<0>(m_colors);
    interpolation0.currentColor.x =
        interpolation0.startColor.x + (interpolation0.endColor.x - interpolation0.startColor.x) * t;
    interpolation0.currentColor.y =
        interpolation0.startColor.y + (interpolation0.endColor.y - interpolation0.startColor.y) * t;
    interpolation0.currentColor.z =
        interpolation0.startColor.z + (interpolation0.endColor.z - interpolation0.startColor.z) * t;
    interpolation0.currentColor.w =
        interpolation0.startColor.w + (interpolation0.endColor.w - interpolation0.startColor.w) * t;

    Interpolation &interpolation1 = std::get<1>(m_colors);
    interpolation1.currentColor.x =
        interpolation1.startColor.x + (interpolation1.endColor.x - interpolation1.startColor.x) * t;
    interpolation1.currentColor.y =
        interpolation1.startColor.y + (interpolation1.endColor.y - interpolation1.startColor.y) * t;
    interpolation1.currentColor.z =
        interpolation1.startColor.z + (interpolation1.endColor.z - interpolation1.startColor.z) * t;
    interpolation1.currentColor.w =
        interpolation1.startColor.w + (interpolation1.endColor.w - interpolation1.startColor.w) * t;

    Interpolation &interpolation2 = std::get<2>(m_colors);
    interpolation2.currentColor.x =
        interpolation2.startColor.x + (interpolation2.endColor.x - interpolation2.startColor.x) * t;
    interpolation2.currentColor.y =
        interpolation2.startColor.y + (interpolation2.endColor.y - interpolation2.startColor.y) * t;
    interpolation2.currentColor.z =
        interpolation2.startColor.z + (interpolation2.endColor.z - interpolation2.startColor.z) * t;
    interpolation2.currentColor.w =
        interpolation2.startColor.w + (interpolation2.endColor.w - interpolation2.startColor.w) * t;

    Interpolation &interpolation3 = std::get<3>(m_colors);
    interpolation3.currentColor.x =
        interpolation3.startColor.x + (interpolation3.endColor.x - interpolation3.startColor.x) * t;
    interpolation3.currentColor.y =
        interpolation3.startColor.y + (interpolation3.endColor.y - interpolation3.startColor.y) * t;
    interpolation3.currentColor.z =
        interpolation3.startColor.z + (interpolation3.endColor.z - interpolation3.startColor.z) * t;
    interpolation3.currentColor.w =
        interpolation3.startColor.w + (interpolation3.endColor.w - interpolation3.startColor.w) * t;

    Interpolation &interpolation4 = std::get<4>(m_colors);
    interpolation4.currentColor.x =
        interpolation4.startColor.x + (interpolation4.endColor.x - interpolation4.startColor.x) * t;
    interpolation4.currentColor.y =
        interpolation4.startColor.y + (interpolation4.endColor.y - interpolation4.startColor.y) * t;
    interpolation4.currentColor.z =
        interpolation4.startColor.z + (interpolation4.endColor.z - interpolation4.startColor.z) * t;
    interpolation4.currentColor.w =
        interpolation4.startColor.w + (interpolation4.endColor.w - interpolation4.startColor.w) * t;

    Interpolation &interpolation5 = std::get<5>(m_colors);
    interpolation4.currentColor.x =
        interpolation4.startColor.x + (interpolation4.endColor.x - interpolation4.startColor.x) * t;
    interpolation4.currentColor.y =
        interpolation4.startColor.y + (interpolation4.endColor.y - interpolation4.startColor.y) * t;
    interpolation4.currentColor.z =
        interpolation4.startColor.z + (interpolation4.endColor.z - interpolation4.startColor.z) * t;
    interpolation4.currentColor.w =
        interpolation4.startColor.w + (interpolation4.endColor.w - interpolation4.startColor.w) * t;

    Interpolation &interpolation6 = std::get<6>(m_colors);
    interpolation4.currentColor.x =
        interpolation4.startColor.x + (interpolation4.endColor.x - interpolation4.startColor.x) * t;
    interpolation4.currentColor.y =
        interpolation4.startColor.y + (interpolation4.endColor.y - interpolation4.startColor.y) * t;
    interpolation4.currentColor.z =
        interpolation4.startColor.z + (interpolation4.endColor.z - interpolation4.startColor.z) * t;
    interpolation4.currentColor.w =
        interpolation4.startColor.w + (interpolation4.endColor.w - interpolation4.startColor.w) * t;

    if (elapsedTime >= m_duration) {
      m_active = false;
    }
  }

  return {std::get<0>(m_colors).currentColor, std::get<1>(m_colors).currentColor, std::get<2>(m_colors).currentColor,
          std::get<3>(m_colors).currentColor, std::get<4>(m_colors).currentColor, std::get<5>(m_colors).currentColor,
          std::get<6>(m_colors).currentColor};
}
