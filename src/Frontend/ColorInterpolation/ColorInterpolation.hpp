#pragma once

#include <algorithm>
#include <chrono>
#include <tuple>
#include <utility>
#include "imgui.h"
#include "Util/Easing/Easing.hpp"

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

class ColorInterpolation {
public:
    ColorInterpolation(const ColorInterpolation &) = delete;
    ColorInterpolation &operator=(const ColorInterpolation &) = delete;

    static ColorInterpolation &GetInstance();

    void ChangeGradientColors(const ImVec4 &GradientStart, const ImVec4 &GradientEnd, const ImVec4 &CircleColor1, const ImVec4 &CircleColor2, const ImVec4 &CircleColor3, const ImVec4 &CircleColor4,
                              const ImVec4 &CircleColor5, float durationSec);

    std::tuple<ImVec4, ImVec4, ImVec4, ImVec4, ImVec4, ImVec4, ImVec4> GetCurrentGradientColors(Infinity::Easing::EasingTypes easing_types);
    // Bg gradient start, Bg Gradient end, Circle color1 ,circle color2, circle color3, circle color 4, circle color5

private:
    ColorInterpolation();

private:
    struct Interpolation {
        ImVec4 startColor;
        ImVec4 endColor;
        ImVec4 currentColor;

        explicit Interpolation(const ImVec4 &start = ImVec4(0, 0, 0, 0), const ImVec4 &end = ImVec4(1, 1, 1, 1), const ImVec4 &current = ImVec4(0.5f, 0.5f, 0.5f, 1.0f)) :
            startColor(start), endColor(end), currentColor(current) {
        }
    };

    std::tuple<Interpolation, Interpolation, Interpolation, Interpolation, Interpolation, Interpolation, Interpolation> m_Colors;

    float m_Duration;
    TimePoint m_StartTime;
    bool m_Active;
};
