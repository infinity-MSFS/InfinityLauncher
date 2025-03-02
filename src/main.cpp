#include <Backend/Updater/Updater.hpp>
#include <filesystem>
#include <iostream>

#include "Backend/Application/Application.hpp"
#include "Backend/Downloads/Downloads.hpp"
#include "Backend/Router/Router.hpp"
#include "Backend/UIHelpers/UiHelpers.hpp"
#include "Frontend/Background/Background.hpp"
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "Frontend/Pages/Home/Home.hpp"
#include "Frontend/SVG/SVGDrawing.hpp"
#include "Frontend/Theme/Theme.hpp"
#include "Util/State/GroupStateManager.hpp"
#include "Util/State/RenderGroupData.hpp"
#include "Util/State/State.hpp"
#include "imgui_internal.h"

#include "Assets/Images/backArrow.h"
#include "Assets/Images/backIcon.h"
#include "Assets/Images/downloadIcon.h"
#include "Assets/Images/settingsIcon.h"
#include "Frontend/Pages/Project/Project.hpp"


bool g_ApplicationRunning = true;

// #define TEST_LOADING_SCREEN

static int downloadID = -1;
static bool pages_registered = false;
static std::vector<unsigned int> active_index = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static std::shared_ptr<Infinity::Image> settingsIcon = nullptr;
static std::shared_ptr<Infinity::Image> backIcon = nullptr;
static std::shared_ptr<Infinity::Image> downloadsIcon = nullptr;


class PageRenderLayer final : public Infinity::Layer {
public:
    void OnAttach() override {
        auto &interpolator = ColorInterpolation::GetInstance();
        interpolator.ChangeGradientColors(Infinity::HomePagePrimary, Infinity::HomePageSecondary, {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f}, {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f},
                                          {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f}, {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f},
                                          {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f}, 1.0f);

        auto &state = Infinity::State::GetInstance();

        Infinity::GroupDataState group_state;
        state.RegisterPageState("main", std::make_shared<Infinity::MainState>(group_state));

        std::thread([] {
            {
                uint32_t w1, h1;
                void *data = Infinity::Image::Decode(g_SettingIcon, sizeof(g_SettingIcon), w1, h1);
                settingsIcon = std::make_shared<Infinity::Image>(w1, h1, Infinity::ImageFormat::RGBA, data);
                free(data);
            }
            {
                uint32_t w2, h2;
                void *data = Infinity::Image::Decode(g_BackIcon, sizeof(g_BackIcon), w2, h2);
                backIcon = std::make_shared<Infinity::Image>(w2, h2, Infinity::ImageFormat::RGBA, data);
                free(data);
            }
            {
                uint32_t w3, h3;
                void *data = Infinity::Image::Decode(g_DownloadIcon, sizeof(g_DownloadIcon), w3, h3);
                downloadsIcon = std::make_shared<Infinity::Image>(w3, h3, Infinity::ImageFormat::RGBA, data);
                free(data);
            }


            auto thread_state = Infinity::State::GetInstance().GetPageState<Infinity::MainState>("main");
            std::shared_ptr<Infinity::MainState> &thread_state_ptr = *thread_state;

            fetch_and_decode_groups(thread_state_ptr);

            Infinity::Home::GetInstance()->RegisterProject("Aero Dynamics", thread_state_ptr->images.groupImages.at("aero_dynamics").projectImages[0].backgroundImage,
                                                           thread_state_ptr->images.groupImages.at("aero_dynamics").logo, 3);
            Infinity::Home::GetInstance()->RegisterProject("Delta Sim", thread_state_ptr->images.groupImages.at("delta_sim").projectImages[0].backgroundImage,
                                                           thread_state_ptr->images.groupImages.at("delta_sim").logo, 4);
            Infinity::Home::GetInstance()->RegisterProject("Lunar Sim", thread_state_ptr->images.groupImages.at("lunar_sim").projectImages[0].backgroundImage,
                                                           thread_state_ptr->images.groupImages.at("lunar_sim").logo, 5);
            Infinity::Home::GetInstance()->RegisterProject("Ouroboros Jets", thread_state_ptr->images.groupImages.at("ouroboros").projectImages[0].backgroundImage,
                                                           thread_state_ptr->images.groupImages.at("ouroboros").logo, 6);
            Infinity::Home::SetLoaded(true);
        }).detach();
    }

    void OnUIRender() override {
        const auto bg = Infinity::Background::GetInstance();
        bg.RenderBackground();

        auto loading_screen = [] {
            {
                for (auto &index: active_index) {
                    if (++index > 241) {
                        index = 0;
                    }
                }
            }
            DrawInfinityLogoAnimated(0.8f, {ImGui::GetWindowWidth() / 2, ImGui::GetWindowHeight() / 2 - 200.0f}, active_index);
        };

        Infinity::Background::UpdateColorScheme();

        auto &state = Infinity::State::GetInstance();

        if (const auto main_state = state.GetPageState<Infinity::MainState>("main"); main_state.has_value() && !Infinity::Home::DoneLoading()) {
            if (const std::shared_ptr<Infinity::MainState> &statePtr = *main_state; statePtr->state.groups.empty()) {
                loading_screen();
                return;
            }
        } else if (main_state.has_value()) {
            const std::shared_ptr<Infinity::MainState> &statePtr = *main_state;
            if (!pages_registered) {
                // Pages 0, 1, and 2 are reserved for home, settings and downloads
                const std::unordered_map<int, std::pair<std::function<void()>, Infinity::Palette>> routes = {
                        {0, {[] { Infinity::Home::GetInstance()->Render(); }, {"#1271FF1C", "#DD4AFF1C", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
                        {1,
                         {[] {
                              ImGui::Text("Settings");
                              if (ImGui::Button("Update")) {
                                  auto pid_dir = Infinity::Updater::GetConfigDir() + "/pid.infinitypid";
                                  Infinity::Updater::WritePidToFile(pid_dir);

                                  auto exe_folder = Infinity::Updater::GetCurrentExecutablePath();
                                  std::string updater_path = exe_folder + "/Updater.exe";
                                  std::string current_exe = exe_folder + "/InfinityLauncher.exe";
                                  std::string new_exe = exe_folder + "/UPDATE_Infinity";
                                  Infinity::Updater::LaunchUpdater(updater_path, new_exe, current_exe, pid_dir);
                              }
                              ImGui::Text("Updated InfinityLauncher :)");
                          },
                          {"#1271FF1C", "#DD4AFF1C", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
                        {2, {[] { ImGui::Text("Downloads"); }, {"#1271FF1C", "#DD4AFF1C", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
                        {3,
                         {[&statePtr] {
                              Infinity::ProjectPage project_page(statePtr->state.groups["aero_dynamics"], statePtr->images.groupImages["aero_dynamics"]);
                              project_page.Render();
                          },
                          {"#050912", "#050505", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
                        {4,
                         {[&statePtr] {
                              Infinity::ProjectPage project_page(statePtr->state.groups["delta_sim"], statePtr->images.groupImages["delta_sim"]);
                              project_page.Render();
                          },
                          {"#4f1a00", "#efaa00", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
                        {5,
                         {[&statePtr] {
                              Infinity::ProjectPage project_page(statePtr->state.groups["lunar_sim"], statePtr->images.groupImages["lunar_sim"]);
                              project_page.Render();
                          },
                          {"#080F19", "#384B5F", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
                        {6,
                         {[&statePtr] {
                              Infinity::ProjectPage project_page(statePtr->state.groups["ouroboros"], statePtr->images.groupImages["ouroboros"]);
                              project_page.Render();
                          },
                          {"#210e3a", "#2a2fff", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}}

                };
                Infinity::Utils::Router::configure(routes);
                Infinity::Utils::Router::getInstance().value()->setPage(0);

                pages_registered = true;
                // ImGui::Begin("Main State");
                // RenderGroupDataState(statePtr->state);
                // ImGui::End();
            }
        }

        if (!Infinity::Home::DoneLoading()) {
            loading_screen();
            return;
        }
#ifdef TEST_LOADING_SCREEN
        loading_screen();
#else
        const auto router = Infinity::Utils::Router::getInstance();
        const int page = (*router)->getPage();
        if (router.has_value()) {
            {
                constexpr int buttonWidth = 24;
                constexpr int buttonHeight = 24;
                ImGui::SetCursorPos(ImVec2(10.0f, 8.0f));
                if (ImGui::InvisibleButton("Home", ImVec2(buttonWidth, buttonHeight))) {
                    router.value()->setPage(page != 0 ? 0 : 1);
                }
                DrawButtonImage(page != 0 ? backIcon : settingsIcon, Infinity::UI::Colors::Theme::text, Infinity::UI::Colors::Theme::text_darker, Infinity::UI::Colors::Theme::text_darker,
                                Infinity::RectExpanded(Infinity::GetItemRect(), 0.0f, 0.0f));
            }
            {
                constexpr int buttonWidth2 = 24;
                constexpr int buttonHeight2 = 24;
                ImGui::SetCursorPos(ImVec2(44.0f, 8.0f));
                if (ImGui::InvisibleButton("downloads", ImVec2(buttonWidth2, buttonHeight2))) {
                    router.value()->setPage(2);
                }
                DrawButtonImage(downloadsIcon, Infinity::UI::Colors::Theme::text, Infinity::UI::Colors::Theme::text_darker, Infinity::UI::Colors::Theme::text_darker, Infinity::GetItemRect());
            }

            router.value()->RenderCurrentPage();
        };
#endif
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
    const auto app = new Infinity::Application{specifications};
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
