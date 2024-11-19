#include <filesystem>
#include <iostream>

#include "Backend/Application/Application.hpp"
#include "Frontend/Background/Background.hpp"
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "imgui_internal.h"

bool g_ApplicationRunning = true;


class PageRenderLayer final : public Infinity::Layer {
public:
    void OnAttach() override {
        auto &interpolator = ColorInterpolation::GetInstance();
        interpolator.ChangeGradientColors(Infinity::HomePagePrimary, Infinity::HomePageSecondary, {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f}, {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f},
                                          {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f}, {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f},
                                          {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f},
                                          1.0f);
    }

    void OnUIRender() override {
        const auto bg = Infinity::Background::GetInstance();
        bg.RenderBackground();
        auto &interpolator = ColorInterpolation::GetInstance();

        Infinity::Background::UpdateColorScheme();

        if (ImGui::Button("Color1")) {
            interpolator.ChangeGradientColors(ImVec4(0.3f, 0.2f, 0.0f, 0.11f), ImVec4(1.0f, 0.3f, 0.2f, 0.11f), {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f},
                                              {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f},
                                              {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f}, {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f},
                                              {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f}, 1.01f);

        }

        if (ImGui::Button("Color2")) {
            interpolator.ChangeGradientColors(ImVec4(0.2f, 0.0f, 0.3f, 0.11f), ImVec4(0.3f, 1.0f, 0.3f, 0.11f), {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f},
                                              {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f},
                                              {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f}, {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f},
                                              {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f}, 1.01f);

        }

        if (ImGui::Button("Default")) {
            interpolator.ChangeGradientColors(Infinity::HomePagePrimary, Infinity::HomePageSecondary, {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f}, {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f},
                                              {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f}, {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f},
                                              {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f},
                                              1.0f);
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
