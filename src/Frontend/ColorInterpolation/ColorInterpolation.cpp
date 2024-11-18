#include "ColorInterpolation.hpp"

ColorInterpolation &ColorInterpolation::GetInstance() {
    static ColorInterpolation instance;
    return instance;
}

ColorInterpolation::ColorInterpolation() :
    m_Color1({ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f)}),
    m_Color2({ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f)}), m_Duration(0.0f), m_Active(false) {}

void ColorInterpolation::ChangeGradientColors(const ImVec4 &endColor1, const ImVec4 &endColor2, float durationSec) {
    m_Color1.startColor = m_Color1.currentColor;
    m_Color1.endColor = endColor1;
    
    m_Color2.startColor = m_Color2.currentColor;
    m_Color2.endColor = endColor2;

    m_Duration = durationSec;
    m_StartTime = Clock::now();
    m_Active = true;
}

std::pair<ImVec4, ImVec4> ColorInterpolation::GetCurrentGradientColors() {
    if (m_Active) {
        float elapsedTime = std::chrono::duration<float>(Clock::now() - m_StartTime).count();
        float t = std::clamp(elapsedTime / m_Duration, 0.0f, 1.0f);

        m_Color1.currentColor.x = m_Color1.startColor.x + (m_Color1.endColor.x - m_Color1.startColor.x) * t;
        m_Color1.currentColor.y = m_Color1.startColor.y + (m_Color1.endColor.y - m_Color1.startColor.y) * t;
        m_Color1.currentColor.z = m_Color1.startColor.z + (m_Color1.endColor.z - m_Color1.startColor.z) * t;
        m_Color1.currentColor.w = m_Color1.startColor.w + (m_Color1.endColor.w - m_Color1.startColor.w) * t;

        m_Color2.currentColor.x = m_Color2.startColor.x + (m_Color2.endColor.x - m_Color2.startColor.x) * t;
        m_Color2.currentColor.y = m_Color2.startColor.y + (m_Color2.endColor.y - m_Color2.startColor.y) * t;
        m_Color2.currentColor.z = m_Color2.startColor.z + (m_Color2.endColor.z - m_Color2.startColor.z) * t;
        m_Color2.currentColor.w = m_Color2.startColor.w + (m_Color2.endColor.w - m_Color2.startColor.w) * t;

        if (elapsedTime >= m_Duration) {
            m_Active = false;
        }
    }

    return {m_Color1.currentColor, m_Color2.currentColor};
}
