#include <filesystem>
#include <iostream>

#include "Backend/Application/Application.hpp"
#include "Frontend/Background/Background.hpp"
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "Frontend/SVG/SVGDrawing.hpp"
#include "Util/State/GroupStateManager.hpp"
#include "Util/State/RenderGroupData.hpp"
#include "Util/State/State.hpp"
#include "imgui_internal.h"
#include "Backend/Downloads/Downloads.hpp"
#include "Backend/Router/Router.hpp"
#include "Frontend/Pages/Home/Home.hpp"

bool g_ApplicationRunning = true;


std::shared_ptr<Infinity::Image> g_TestImage;
static int downloadID = -1;
static bool pages_registered = false;


class PageRenderLayer final : public Infinity::Layer {
public:
    void OnAttach() override {
        auto &interpolator = ColorInterpolation::GetInstance();
        interpolator.ChangeGradientColors(Infinity::HomePagePrimary, Infinity::HomePageSecondary,
                                          {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f},
                                          {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f},
                                          {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f},
                                          {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f},
                                          {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f}, 1.0f);

        auto &state = Infinity::State::GetInstance();

        Infinity::GroupDataState group_state;
        state.RegisterPageState("main", std::make_shared<Infinity::MainState>(group_state));

        std::thread([] {


            auto thread_state = Infinity::State::GetInstance().GetPageState<Infinity::MainState>("main");
            std::shared_ptr<Infinity::MainState> &thread_state_ptr = *thread_state;

            Infinity::fetch_and_decode_groups(thread_state_ptr);

            g_TestImage = Infinity::Image::LoadFromURL(thread_state_ptr->state.groups["aero_dynamics"].projects[0].background);


        }).detach();


    }

    void OnUIRender() override {
        const auto bg = Infinity::Background::GetInstance();
        bg.RenderBackground();
        auto &interpolator = ColorInterpolation::GetInstance();


        Infinity::Background::UpdateColorScheme();

        auto &state = Infinity::State::GetInstance();
        auto main_state = state.GetPageState<Infinity::MainState>("main");


        if (main_state.has_value()) {
            if (const std::shared_ptr<Infinity::MainState> &statePtr = *main_state; statePtr->state.groups.empty()) {
                ImGui::Text("Loading...");
                return;
            }

            const std::shared_ptr<Infinity::MainState> &statePtr = *main_state;
            if (!pages_registered) {
                const std::unordered_map<int, std::pair<std::function<void()>, Infinity::Palette>> routes = {
                        {
                                0, {[] { Infinity::Home::GetInstance()->Render(); }, {"#1271FF1C", "#DD4AFF1C", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}
                        },
                        {
                                1, {[] {
                                        ImGui::Text("Aero dynamics");
                                        if (ImGui::Button("Home")) {
                                            auto router = Infinity::Utils::Router::getInstance().value()->setPage(0);
                                        }
                                    },
                                    {"#050912", "#050505", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}
                        },
                        {
                                2, {[] {
                                        ImGui::Text("Delta Sim");
                                        if (ImGui::Button("Home")) {
                                            auto router = Infinity::Utils::Router::getInstance().value()->setPage(0);
                                        }
                                    },
                                    {"#4f1a00", "#efaa00", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}
                        },
                        {
                                3, {[] {
                                        ImGui::Text("Lunar Sim");
                                        if (ImGui::Button("Home")) {
                                            auto router = Infinity::Utils::Router::getInstance().value()->setPage(0);
                                        }
                                    },
                                    {"#080F19", "#384B5F", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}
                        },
                        {
                                4, {[] {
                                        ImGui::Text("Ouroboros Jets");
                                        if (ImGui::Button("Home")) {
                                            auto router = Infinity::Utils::Router::getInstance().value()->setPage(0);
                                        }
                                    },
                                    {"#210e3a", "#2a2fff", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}
                        }

                };
                Infinity::Utils::Router::configure(routes);


                Infinity::Home::GetInstance()->RegisterProject("Aero Dynamics", statePtr->state.groups.at("aero_dynamics").projects[0].background, statePtr->state.groups.at("aero_dynamics").logo, 1);
                Infinity::Home::GetInstance()->RegisterProject("Delta Sim", statePtr->state.groups.at("delta_sim").projects[0].background, statePtr->state.groups.at("delta_sim").logo, 2);
                Infinity::Home::GetInstance()->RegisterProject("Lunar Sim", statePtr->state.groups.at("lunar_sim").projects[0].background, statePtr->state.groups.at("lunar_sim").logo, 3);
                Infinity::Home::GetInstance()->RegisterProject("Ouroboros Jets", statePtr->state.groups.at("ouroboros").projects[0].background, statePtr->state.groups.at("ouroboros").logo, 4);
                pages_registered = true;

            }
            // ImGui::Begin("Main State");
            // RenderGroupDataState(statePtr->state);
            // ImGui::End();


        }

        // if (g_TestImage) {
        //     Infinity::Image::RenderImage(g_TestImage, {50.0f, 50.0f}, 0.2f);
        //     Infinity::Image::RenderImage(g_TestImage, {50.0f, 20.0f}, {500.0f, 900.0f});
        // }

        auto router = Infinity::Utils::Router::getInstance();
        if (router.has_value()) {
            router.value()->RenderCurrentPage();
        }


        // auto &downloader = Infinity::Downloads::GetInstance();
        // if
        // (ImGui::Button("Download")) {
        //     downloadID = downloader.StartDownload("http://link.testfile.org/150MB", "/home/katelyn/test/test_file.zip");
        // }
        //
        // if
        // (downloadID != -1) {
        //     auto *downloadDataPtr = downloader.GetDownloadData(downloadID);
        //     ImGui::Text("Downloading...");
        //     ImGui::Text("Download ID: %d", downloadDataPtr->zoe->state());
        //
        //     if (ImGui::Button("Pause")) {
        //         downloader.PauseDownload(downloadID);
        //     }
        //     if (ImGui::Button("Resume")) {
        //         downloader.ResumeDownload(downloadID);
        //     }
        //
        //     if (downloadDataPtr) {
        //         ImGui::Text("Download Progress: %.2f", downloadDataPtr->progress);
        //     }
        // }


        // DrawLeftLogoHalf(0.5f, {50.0f, 50.0f});
        // DrawLogoRightHalf(0.5f, {50.0f, 50.0f});
        // if (ImGui::Button("Color1")) {
        //     interpolator.ChangeGradientColors(ImVec4(0.3f, 0.2f, 0.0f, 0.11f), ImVec4(1.0f, 0.3f, 0.2f, 0.11f),
        //                                       {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f},
        //                                       {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f},
        //                                       {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f},
        //                                       {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f},
        //                                       {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f}, 1.01f);
        // }
        //
        // if (ImGui::Button("Color2")) {
        //     interpolator.ChangeGradientColors(ImVec4(0.2f, 0.0f, 0.3f, 0.11f), ImVec4(0.3f, 1.0f, 0.3f, 0.11f),
        //                                       {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f},
        //                                       {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f},
        //                                       {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f},
        //                                       {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f},
        //                                       {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f}, 1.01f);
        // }
        //
        // if (ImGui::Button("Default")) {
        //     interpolator.ChangeGradientColors(Infinity::HomePagePrimary, Infinity::HomePageSecondary,
        //                                       {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f},
        //                                       {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f},
        //                                       {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f},
        //                                       {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f},
        //                                       {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f}, 1.0f);
        // }
        //
        // ImGui::Text("The Monkeys will win the war");
    }

};

Infinity::Application *Infinity::CreateApplication(int argc, char **argv) {
    const std::filesystem::path path = "Assets/Images/Logo.h";
    const ApplicationSpecifications specifications = ApplicationSpecifications{"Infinity Launcher",
                                                                               std::make_pair(1440, 1026),
                                                                               std::make_pair(3840, 2160),
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

namespace
Infinity {
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

int main(const int argc, char **argv) {
    Infinity::Main(argc, argv);
}

#endif
