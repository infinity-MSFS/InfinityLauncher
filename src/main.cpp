#include <filesystem>
#include <iostream>

#include "Backend/Application/Application.hpp"
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "imgui_internal.h"

bool g_ApplicationRunning = true;

void RenderGradientRect(ImDrawList *draw_list, const ImVec2 &pos, const ImVec2 &size, ImVec4 col_start, ImVec4 col_end, bool horizontal = true) {

    ImVec2 top_left = pos;
    ImVec2 bottom_right = ImVec2(pos.x + size.x, pos.y + size.y);

    const int segments = 100;

    float step_x = size.x / float(segments);
    float step_y = size.y / float(segments);

    for (int i = 0; i < segments; i++) {
        float t = i / float(segments - 1);
        ImU32 col = ImGui::GetColorU32(ImLerp(col_start, col_end, t));

        ImVec2 segment_min, segment_max;
        if (horizontal) {
            segment_min = ImVec2(top_left.x + (step_x * i), top_left.y);
            segment_max = ImVec2(top_left.x + (step_x * (i + 1)), bottom_right.y);
        } else {
            segment_min = ImVec2(top_left.x, top_left.y + (step_y * i));
            segment_max = ImVec2(bottom_right.x, top_left.y + (step_y * (i + 1)));
        }

        draw_list->AddRectFilled(segment_min, segment_max, col);
    }
}

class PageRenderLayer final : public Infinity::Layer {
public:
    void OnAttach() override {
        auto &interpolator = ColorInterpolation::GetInstance();
        interpolator.ChangeGradientColors(ImVec4(0.0f, 0.3f, 0.2f, 1.0f), ImVec4(0.3f, 0.2f, 0.0f, 1.0f), 1.0f);
    }

    void OnUIRender() override {
        auto &interpolator = ColorInterpolation::GetInstance();


        auto [color1, color2] = interpolator.GetCurrentGradientColors();

        RenderGradientRect(ImGui::GetWindowDrawList(), {0.0f, 0.0f}, ImGui::GetWindowSize(), color1, color2);

        if (ImGui::Button("Color1")) {
            interpolator.ChangeGradientColors(ImVec4(0.0f, 0.3f, 0.2f, 1.0f), ImVec4(0.3f, 0.2f, 0.0f, 1.0f), 1.0f);
        }

        if (ImGui::Button("Color2")) {
            interpolator.ChangeGradientColors(ImVec4(0.2f, 0.0f, 0.3f, 1.0f), ImVec4(0.3f, 0.3f, 0.3f, 1.0f), 1.0f);
        }

        ImGui::Text("The Monkeys will win the war");
    }
};

Infinity::Application *Infinity::CreateApplication(int argc, char **argv) {
    const std::filesystem::path path = "Assets/Images/Logo.h";
    const ApplicationSpecifications specifications = ApplicationSpecifications{"Infinity Launcher",
                                                                               std::make_pair(1440, 1026),
                                                                               std::make_pair(2560, 1440),
                                                                               std::make_pair(1240, 680),
                                                                               path,
                                                                               true,
#ifdef WIN32
                                                                               true,
#else
                                                                               false,
#endif
                                                                               true};
    const auto app = new Application{specifications};
    app->PushLayer<PageRenderLayer>();

    return app;
}

namespace Infinity {
    int Main(const int argc, char **argv) {
        while (g_ApplicationRunning) {
            const auto app = CreateApplication(argc, argv);
            app->Run();
            delete app;
            g_ApplicationRunning = false;
        }
        return 0;
    }
} // namespace Infinity


#if defined(RELEASE_DIST) && WIN32

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nShowCmd) { Infinity::Main(__argc, __argv); }

#else

int main(const int argc, char **argv) { Infinity::Main(argc, argv); }

#endif
