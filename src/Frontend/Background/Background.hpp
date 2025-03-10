
#pragma once


#include <cmath>
#include <iostream>
#include <string>
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "imgui.h"
#include "vector"


namespace Infinity {

    const ImVec4 HomePagePrimary(18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.11f);
    const ImVec4 HomePageSecondary(221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.11f);

    static std::vector<ImVec2> m_circlePos;


    class Background {
    public:
        static Background &GetInstance() {
            static Background instance;
            return instance;
        }

        Background();

        static void SetDotOpacity(float opacity);

        void RenderBackground();

        static void UpdateColorScheme() {
            auto &interpolator = ColorInterpolation::GetInstance();

            constexpr auto easing_types = Easing::EasingTypes::EaseInOutCubic;

            const auto colors = interpolator.GetCurrentGradientColors(easing_types);
            m_primaryColor = std::get<0>(colors);
            m_secondaryColor = std::get<1>(colors);
            m_circleColor1 = std::get<2>(colors);
            m_circleColor2 = std::get<3>(colors);
            m_circleColor3 = std::get<4>(colors);
            m_circleColor4 = std::get<5>(colors);
            m_circleColor5 = std::get<6>(colors);
        }

        static void SetHomePage(bool state) {
            if (state) {
                auto &interpolator = ColorInterpolation::GetInstance();
                interpolator.ChangeGradientColors(HomePagePrimary, HomePageSecondary, {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.01f}, {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.01f},
                                                  {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.01f}, {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.01f},
                                                  {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.01f}, 1.0f);
                m_HomePage = true;
            } else {
                m_HomePage = state;
            }
        }

    private:
        void UpdateDotOpacity();

        static void RenderBackgroundDotsLayer();

        static void RenderBackgroundGradientLayer();

        static void RenderBackgroundBaseLayer();

        static void RenderGradientCircle(ImVec2 center, float radius, float maxOpacity, ImU32 color);

        static void InitializeCirclePosition(const int index, const ImVec2 position) {
            if (index >= 0 && index < m_circlePos.size() && m_circlePos[index].x == 0 && m_circlePos[index].y == 0) {
                m_circlePos[index] = position;
            }
        }


        static void TrySetDefaultPositions();

        static ImVec2 GetCircleCoords(const float radius, const float theta, const ImVec2 center) {
            const float radians = (3.141592f / 180.0f) * theta;
            float x = center.x + radius * cosf(radians);
            float y = center.y + radius * sinf(radians);

            return {x, y};
        }

    private:
        static ImVec2 m_windowPos;
        static ImVec2 m_windowSize;

        static ImVec4 m_primaryColor;
        static ImVec4 m_secondaryColor;
        static ImVec4 m_circleColor1;
        static ImVec4 m_circleColor2;
        static ImVec4 m_circleColor3;
        static ImVec4 m_circleColor4;
        static ImVec4 m_circleColor5;
        static bool m_HomePage;
        static float m_dotOpacity;
        static float m_targetDotOpacity;
    };


} // namespace Infinity
