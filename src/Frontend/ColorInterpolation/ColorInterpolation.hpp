#pragma once

#include <algorithm>
#include <chrono>
#include <utility>
#include "imgui.h"

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

class ColorInterpolation {
public:
    ColorInterpolation(const ColorInterpolation &) = delete;
    ColorInterpolation &operator=(const ColorInterpolation &) = delete;

    static ColorInterpolation &GetInstance();

    void ChangeGradientColors(const ImVec4 &endColor1, const ImVec4 &endColor2, float durationSec);

    std::pair<ImVec4, ImVec4> GetCurrentGradientColors();

private:
    ColorInterpolation();

private:
    struct Interpolation {
        ImVec4 startColor;
        ImVec4 endColor;
        ImVec4 currentColor;
    };

    Interpolation m_Color1;
    Interpolation m_Color2;

    float m_Duration;
    TimePoint m_StartTime;
    bool m_Active;
};
