#include <Backend/Updater/Updater.hpp>
#include <filesystem>
#include <iostream>

#include "GL/glew.h"
//

#include "Assets/Images/backArrow.h"
#include "Assets/Images/backIcon.h"
#include "Assets/Images/downloadIcon.h"
#include "Assets/Images/settingsIcon.h"
#include "Assets/Images/test-tube.h"
#include "Backend/Application/Application.hpp"
#include "Backend/Downloads/Downloads.hpp"
#include "Backend/HWID/Hwid.hpp"
#include "Backend/Image/SvgImage.hpp"
#include "Backend/Layer/Layer.hpp"
#include "Backend/Router/Router.hpp"
#include "Backend/UIHelpers/UiHelpers.hpp"
#include "Frontend/Background/Background.hpp"
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "Frontend/Pages/Betas/Betas.hpp"
#include "Frontend/Pages/Downloads/Downloads.hpp"
#include "Frontend/Pages/Home/Home.hpp"
#include "Frontend/Pages/Project/Project.hpp"
#include "Frontend/Pages/Settings/Settings.hpp"
#include "Frontend/SVG/SVGDrawing.hpp"
#include "Frontend/Theme/Theme.hpp"
#include "Util/State/GroupStateManager.hpp"
#include "Util/State/RenderGroupData.hpp"
#include "Util/State/State.hpp"
#include "imgui_internal.h"


bool g_ApplicationRunning = true;

// #define TEST_LOADING_SCREEN

static int downloadID = -1;
static bool pages_registered = false;
static std::vector<unsigned int> active_index = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static std::shared_ptr<Infinity::Image> settingsIcon = nullptr;
static std::shared_ptr<Infinity::Image> backIcon = nullptr;
static std::shared_ptr<Infinity::Image> downloadsIcon = nullptr;
static std::shared_ptr<Infinity::Image> betaIcon = nullptr;

class PageRenderLayer final : public Infinity::Layer {
  public:
  void OnAttach() override {
    auto &interpolator = ColorInterpolation::GetInstance();
    interpolator.ChangeGradientColors(
        Infinity::HomePagePrimary, Infinity::HomePageSecondary, {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f},
        {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f}, {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f},
        {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f}, {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f},
        1.0f);

    auto &state = Infinity::State::GetInstance();

    Infinity::GroupDataState group_state;
    state.RegisterPageState("main", std::make_shared<Infinity::MainState>(group_state));

    std::thread([] {
      settingsIcon = Infinity::Image::LoadFromMemory(g_SettingIcon, sizeof(g_SettingIcon));
      backIcon = Infinity::Image::LoadFromMemory(g_BackIcon, sizeof(g_BackIcon));
      downloadsIcon = Infinity::Image::LoadFromMemory(g_DownloadIcon, sizeof(g_DownloadIcon));
      betaIcon = Infinity::Image::LoadFromMemory(g_TestTubeIcon, sizeof(g_TestTubeIcon));


      auto thread_state = Infinity::State::GetInstance().GetPageState<Infinity::MainState>("main");
      std::shared_ptr<Infinity::MainState> &thread_state_ptr = *thread_state;

      fetch_and_decode_groups(thread_state_ptr);

      Infinity::Home::GetInstance()->RegisterProject(
          "Aero Dynamics", thread_state_ptr->images.groupImages.at("aero_dynamics").projectImages[0].backgroundImage,
          thread_state_ptr->images.groupImages.at("aero_dynamics").logo, 3);
      Infinity::Home::GetInstance()->RegisterProject(
          "Delta Sim", thread_state_ptr->images.groupImages.at("delta_sim").projectImages[0].backgroundImage,
          thread_state_ptr->images.groupImages.at("delta_sim").logo, 4);
      Infinity::Home::GetInstance()->RegisterProject(
          "Lunar Sim", thread_state_ptr->images.groupImages.at("lunar_sim").projectImages[0].backgroundImage,
          thread_state_ptr->images.groupImages.at("lunar_sim").logo, 5);
      Infinity::Home::GetInstance()->RegisterProject(
          "Ouroboros Jets", thread_state_ptr->images.groupImages.at("ouroboros").projectImages[0].backgroundImage,
          thread_state_ptr->images.groupImages.at("ouroboros").logo, 6);
      Infinity::Home::GetInstance()->RegisterProject(
          "QBit Sim", thread_state_ptr->images.groupImages.at("qbitsim").projectImages[0].backgroundImage,
          thread_state_ptr->images.groupImages.at("qbitsim").logo, 7);
      Infinity::Home::SetLoaded(true);
    }).detach();
  }

  void OnUIRender() override {
    auto bg = Infinity::Background::GetInstance();
    if (auto router = Infinity::Utils::Router::getInstance(); router.has_value()) {
      if (router.value()->getPage() != 3) {
        bg->RenderBackground();
      }
    }


    auto loading_screen = [] {
      {
        for (auto &index: active_index) {
          if (++index > 241) {
            index = 0;
          }
        }
      }
      DrawInfinityLogoAnimated(0.8f, {ImGui::GetWindowWidth() / 2, ImGui::GetWindowHeight() / 2 - 200.0f},
                               active_index);
    };

    bg->UpdateColorScheme();

    auto &state = Infinity::State::GetInstance();

    if (const auto main_state = state.GetPageState<Infinity::MainState>("main");
        main_state.has_value() && !Infinity::Home::DoneLoading()) {
      if (const std::shared_ptr<Infinity::MainState> &statePtr = *main_state; statePtr->state.groups.empty()) {
        bg->RenderBackground();
        loading_screen();
        return;
      }
    } else if (main_state.has_value()) {
      const std::shared_ptr<Infinity::MainState> &statePtr = *main_state;
      if (!pages_registered) {
        // Pages 0, 1, 2 and 3 are reserved for home, settings, downloads and betas
        const std::unordered_map<int, std::pair<std::function<void()>, Infinity::Palette>> routes = {
            {0,
             {[] { Infinity::Home::GetInstance()->Render(); },
              {"#1271FF1C", "#DD4AFF1C", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
            {1,
             {[] {
                Infinity::Settings settings;
                settings.Render();
              },
              {"#1271FF1C", "#DD4AFF1C", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
            {2,
             {[] {
                Downloads downloads;
                downloads.Render();
              },
              {"#1271FF1C", "#DD4AFF1C", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
            {3,
             {[] {
                Infinity::Betas betas;
                betas.Render();

                ImGui::Text("Beta");
              },
              {"#1271FF1C", "#DD4AFF1C", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
            {4,
             {[statePtr] {
                Infinity::ProjectPage project_page(
                    std::make_shared<Infinity::GroupData>(statePtr->state.groups["aero_dynamics"]),
                    std::make_shared<Infinity::GroupDataImages>(statePtr->images.groupImages["aero_dynamics"]));
                project_page.Render();
              },
              {"#050912", "#050505", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
            {5,
             {[statePtr] {
                Infinity::ProjectPage project_page(
                    std::make_shared<Infinity::GroupData>(statePtr->state.groups["delta_sim"]),
                    std::make_shared<Infinity::GroupDataImages>(statePtr->images.groupImages["delta_sim"]));
                project_page.Render();
              },
              {"#4f1a00", "#efaa00", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
            {6,
             {[statePtr] {
                Infinity::ProjectPage project_page(
                    std::make_shared<Infinity::GroupData>(statePtr->state.groups.at("lunar_sim")),
                    std::make_shared<Infinity::GroupDataImages>(statePtr->images.groupImages.at("lunar_sim")));
                project_page.Render();
              },
              {"#080F19", "#384B5F", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
            {7,
             {[statePtr] {
                Infinity::ProjectPage project_page(
                    std::make_shared<Infinity::GroupData>(statePtr->state.groups["ouroboros"]),
                    std::make_shared<Infinity::GroupDataImages>(statePtr->images.groupImages["ouroboros"]));
                project_page.Render();
              },
              {"#210e3a", "#2a2fff", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}},
            {8,
             {[statePtr] {
                Infinity::ProjectPage project_page(
                    std::make_shared<Infinity::GroupData>(statePtr->state.groups["qbitsim"]),
                    std::make_shared<Infinity::GroupDataImages>(statePtr->images.groupImages["qbitsim"]));
                project_page.Render();
              },
              {"#210e3a", "#2a2fff", "#1271FF02", "#DD4AFF02", "#64DCFF02", "#C8323202", "#B4B43202"}}}

        };
        Infinity::Utils::Router::configure(routes);
        Infinity::Utils::Router::getInstance().value()->setPage(0);

        pages_registered = true;
      }
    }

    if (!Infinity::Home::DoneLoading()) {
      bg->RenderBackground();
      loading_screen();

      return;
    }
#ifdef TEST_LOADING_SCREEN
    loading_screen();
#else

    // if (const auto main_state =
    // state.GetPageState<Infinity::MainState>("main"); main_state.has_value())
    // {
    //     main_state.value()->PrintState();
    // }

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
        DrawButtonImage(page != 0 ? backIcon : settingsIcon, Infinity::UI::Colors::Theme::text,
                        Infinity::UI::Colors::Theme::text_darker, Infinity::UI::Colors::Theme::text_darker,
                        Infinity::RectExpanded(Infinity::GetItemRect(), 0.0f, 0.0f));
      }
      {
        constexpr int buttonWidth2 = 24;
        constexpr int buttonHeight2 = 24;
        ImGui::SetCursorPos(ImVec2(44.0f, 8.0f));
        if (ImGui::InvisibleButton("downloads", ImVec2(buttonWidth2, buttonHeight2))) {
          router.value()->setPage(2);
        }
        DrawButtonImage(downloadsIcon, Infinity::UI::Colors::Theme::text, Infinity::UI::Colors::Theme::text_darker,
                        Infinity::UI::Colors::Theme::text_darker, Infinity::GetItemRect());
      }

      if (auto main_state = state.GetPageState<Infinity::MainState>("main");
          main_state.has_value() && (*main_state)->beta_auth) {
        constexpr int buttonWidth3 = 24;
        constexpr int buttonHeight3 = 24;
        ImGui::SetCursorPos(ImVec2(78.0f, 8.0f));
        if (ImGui::InvisibleButton("button_beta_home", ImVec2(buttonWidth3, buttonHeight3))) {
          router.value()->setPage(3);
        }
        DrawButtonImage(betaIcon, Infinity::UI::Colors::Theme::text, Infinity::UI::Colors::Theme::text_darker,
                        Infinity::UI::Colors::Theme::text_darker, Infinity::GetItemRect());
      }

      // ImGui::Begin("FPS Counter");
      // ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
      // ImGui::End();


      router.value()->RenderCurrentPage();
    };
#endif
  }

  void OnDetach() override {}

  void OnUpdate(float ts) override {}
};


namespace Infinity {
  void Main(const int argc, char **argv) {
    while (g_ApplicationRunning) {
      const auto app = Application::CreateApplication(argc, argv, std::make_unique<PageRenderLayer>());
      app->Run();
      g_ApplicationRunning = false;
    }
  }
}  // namespace Infinity


#if defined(RELEASE_DIST) && WIN32

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nShowCmd) {
  Infinity::Main(__argc, __argv);
}

#else

int main(const int argc, char **argv) { Infinity::Main(argc, argv); }

#endif
