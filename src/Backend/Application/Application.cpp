#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#ifdef INFINITY_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif INFINITY_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#elif INFINITY_X11
#define GLFW_EXPOSE_NATIVE_X11
#endif

#include "Application.hpp"

#include "GL/glew.h"
//
#include <GL/gl.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <utility>

#include "Assets/Fonts/Roboto-Bold.h"
#include "Assets/Fonts/Roboto-Italic.h"
#include "Assets/Fonts/Roboto-Regular.h"
#include "Assets/Images/InfinityAppIcon.h"
#include "Assets/Images/logo.h"
#include "Assets/Images/windowIcons.h"
#include "Backend/SystemTray/SystemTray.hpp"
#include "Backend/TextureQueue/TextureQueue.hpp"
#include "Backend/UIHelpers/UiHelpers.hpp"
#include "Frontend/Theme/Theme.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "stb_image/stb_image.h"

#ifdef WIN32
#include <Windows.h>
#include <timeapi.h>
#pragma comment(lib, "Winmm.lib")
#endif


#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif


namespace Infinity {
  Application *Application::s_instance = nullptr;

  Application::Application(const ApplicationSpecifications &specifications)
      : m_specification(specifications)
      , m_window(nullptr) {
    if (auto result = Init(); !result.has_value()) {
      result.error().Dispatch();
    }
    s_instance = this;
  }

  Application::~Application() {
    if (auto result = Shutdown(); !result.has_value()) {
      result.error().Dispatch();
    }
  }

  std::optional<Application *> Application::Get() {
    if (s_instance == nullptr) {
      return std::nullopt;
    }
    return s_instance;
  }

  void Application::GLFWErrorCallback(int error, const char *description) {
    std::cerr << "GLFW Error " << error << ": " << description << "\n";
  }


  std::expected<void, Errors::Error> Application::Init() {
    std::cout << "Initializing" << std::endl;

    glfwSetErrorCallback(GLFWErrorCallback);

    if (!glfwInit()) {
      return std::unexpected(Errors::Error(Errors::ErrorType::Fatal, "Failed to initialize GLFW"));
    }

    // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#ifdef WIN32
    if (m_specification.custom_titlebar) {
      glfwWindowHint(GLFW_TITLEBAR, GLFW_FALSE);
    }
#endif
    const auto version = SetupGLVersion();

    m_window = glfwCreateWindow(static_cast<int>(m_specification.window_size.first),
                                static_cast<int>(m_specification.window_size.second), m_specification.name.c_str(),
                                nullptr, nullptr);

    if (m_window == nullptr) {
      return std::unexpected(Errors::Error(Errors::ErrorType::Fatal, "Failed to create window"));
    }


    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(0);

    glfwSetWindowUserPointer(m_window, this);

#ifdef WIN32
    glfwSetTitlebarHitTestCallback(m_window, [](GLFWwindow *window, int x, int y, int *hit) {
      const auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
      *hit = app->IsTitleBarHovered();
    });
#endif

    GLenum err = glewInit();
    if (err != GLEW_OK) {
      std::cout << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
      return std::unexpected(Errors::Error(Errors::ErrorType::Fatal, "Failed to initialize GLEW"));
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    UI::SetInfinityTheme();

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(6.0f, 6.0f);
    style.ChildRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(version);

    ImFontConfig font_config;
    font_config.FontDataOwnedByAtlas = false;
    ImFont *roboto = io.Fonts->AddFontFromMemoryTTF(g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &font_config);
    m_Fonts["Default"] = roboto;
    m_Fonts["Bold"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoBold, sizeof(g_RobotoBold), 20.0f, &font_config);
    m_Fonts["Italic"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoItalic, sizeof(g_RobotoItalic), 20.0f, &font_config);
    m_Fonts["DefaultLarge"] =
        io.Fonts->AddFontFromMemoryTTF(g_RobotoRegular, sizeof(g_RobotoRegular), 32.0f, &font_config);
    m_Fonts["DefaultXLarge"] =
        io.Fonts->AddFontFromMemoryTTF(g_RobotoRegular, sizeof(g_RobotoRegular), 48.0f, &font_config);
    m_Fonts["BoldLarge"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoBold, sizeof(g_RobotoBold), 32.0f, &font_config);
    m_Fonts["BoldXLarge"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoBold, sizeof(g_RobotoBold), 48.0f, &font_config);
    m_Fonts["h1"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoBold, sizeof(g_RobotoBold), 32.0f, &font_config);
    m_Fonts["h2"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoBold, sizeof(g_RobotoBold), 24.0f, &font_config);
    m_Fonts["h3"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoBold, sizeof(g_RobotoBold), 20.0f, &font_config);

    io.FontDefault = roboto;

    {
      std::shared_ptr<Image> close_image = Image::LoadFromMemory(g_WindowCloseIcon, sizeof(g_WindowCloseIcon));
      m_icon_close = close_image;
    }
    {
      std::shared_ptr<Image> logo = Image::LoadFromMemory(g_infAppIconTransparent, sizeof(g_infAppIconTransparent));
      m_app_header_icon = logo;
    }
    {
      std::shared_ptr<Image> minimize_image = Image::LoadFromMemory(g_WindowMinimizeIcon, sizeof(g_WindowMinimizeIcon));
      m_icon_minimize = minimize_image;
    }
    {
      std::shared_ptr<Image> maximize_image = Image::LoadFromMemory(g_WindowMaximizeIcon, sizeof(g_WindowMaximizeIcon));
      m_icon_maximize = maximize_image;
    }
    {
      std::shared_ptr<Image> restore_image = Image::LoadFromMemory(g_WindowRestoreIcon, sizeof(g_WindowRestoreIcon));
      m_icon_restore = restore_image;
    }


    return {};
  }
  const char *Application::SetupGLVersion() {
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char *glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
    // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

    return glsl_version;
  }

  std::expected<void, Errors::Error> Application::Shutdown() {
    std::cout << "Shutting down" << std::endl;
    m_layer->OnDetach();
    m_app_header_icon.reset();
    m_icon_close.reset();
    m_icon_minimize.reset();
    m_icon_maximize.reset();
    m_icon_restore.reset();


    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();


    glfwDestroyWindow(m_window);
    glfwTerminate();

    m_running = false;

    return {};
  }

  std::expected<void, Errors::Error> Application::Run() {
    m_running = true;


    SystemTray system_tray(m_window);

    const auto clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImGuiIO &io = ImGui::GetIO();

    io.IniFilename = nullptr;

    system_tray.run();


#ifdef WIN32
    timeBeginPeriod(1);
#endif

    double last_frame_time = glfwGetTime();

    while (!glfwWindowShouldClose(m_window) && m_running) {
      glfwPollEvents();
      {
        std::scoped_lock lock(m_event_queue_mutex);

        while (!m_event_queue.empty()) {
          auto &func = m_event_queue.front();
          func();
          m_event_queue.pop();
        }
      }
      m_layer->OnUpdate(m_time_step);
      ProcessImageQueue();

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        const ImVec2 windowPos = viewport->Pos;
        ImGui::SetNextWindowPos(ImVec2(windowPos.x - 1, windowPos.y));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x + 1, viewport->Size.y));
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        if (!m_specification.custom_titlebar && m_menubar_callback) window_flags |= ImGuiWindowFlags_MenuBar;


        ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
        if (m_specification.custom_titlebar) {
          float titleBarHeight;
          DrawTitleBar(titleBarHeight);
        }

        m_layer->OnUIRender();


        ImGui::PopStyleVar(3);

        ImGui::End();
      }

      ImGui::Render();

      int display_w, display_h;
      glfwGetFramebufferSize(m_window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      glfwSwapBuffers(m_window);

      const float time = GetTime();
      m_frame_time = time - m_last_frame_time;
#ifdef INFINITY_WINDOWS
      m_time_step = min(m_frame_time, 0.0333f);
#else
      m_time_step = std::min(m_frame_time, 0.0333f);
#endif
      m_last_frame_time = time;


      CapFPS(last_frame_time);
    }
#ifdef WIN32
    timeEndPeriod(1);
#endif
    return {};
  }

  void Application::Close() { m_running = false; }

  float Application::GetTime() { return static_cast<float>(glfwGetTime()); }

  bool Application::IsMaximized() const { return static_cast<bool>(glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED)); }

  ImFont *Application::GetFont(const std::string &name) {
    if (!s_instance->m_Fonts.contains(name)) {
      return nullptr;
    }
    return s_instance->m_Fonts.at(name);
  }

  void Application::SetWindowIcon(GLFWwindow *window, const unsigned char *data, const int size) {
    GLFWimage images[1];
    images[0].pixels = stbi_load_from_memory(data, size, &images[0].width, &images[0].height, nullptr, 4);

    if (images[0].pixels == nullptr) {
      std::cerr << "Failed to load image" << std::endl;
      return;
    }

    glfwSetWindowIcon(s_instance->m_window, 1, images);
    stbi_image_free(images[0].pixels);
  }

  void Application::SetWindowTitle(const std::string &title) {
    glfwSetWindowTitle(s_instance->m_window, title.c_str());
  }

  void Application::DrawMenubar() const {
    if (!m_menubar_callback) return;

    if (m_specification.custom_titlebar) {
      const ImRect menuBarRect = {
          ImGui::GetCursorPos(),
          {ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing()}};

      ImGui::BeginGroup();
      if (BeginMenubar(menuBarRect)) {
        m_menubar_callback();
      }

      EndMenubar();
      ImGui::EndGroup();
    } else {
      if (ImGui::BeginMenuBar()) {
        m_menubar_callback();
        ImGui::EndMenuBar();
      }
    }
  }

  void Application::CapFPS(double &last_frame_time) const {
    int fps;
    if (m_reduce_fps_on_idle) {
      if (glfwGetWindowAttrib(m_window, GLFW_FOCUSED)) {
        fps = m_fps_cap;
      } else {
        fps = 30;
      }
    }

    double frame_duration = 1.0 / fps;

    double target_time = last_frame_time + frame_duration;
    double current_time = glfwGetTime();
    double wait_time = target_time - current_time;

    if (wait_time > 0.002) {
      std::this_thread::sleep_for(std::chrono::duration<double>(wait_time - 0.002));
    }

    while (glfwGetTime() < target_time) {
      std::this_thread::yield();
    }

    last_frame_time = target_time;
  }


  void Application::ProcessImageQueue() {
    std::vector<std::shared_ptr<Image>> toProcess;

    {
      std::lock_guard lock(g_texture_queue_mutex);
      toProcess.swap(g_texture_creation_queue);
    }

    for (auto &image: toProcess) {
      image->CreateGLTexture();
    }
  }


  void Application::DrawTitleBar(float &out_title_bar_height) {
    constexpr float title_bar_height = 40.0f;
    const bool isMaximized = IsMaximized();
    const float title_bar_vertical_offset = isMaximized ? -6.0f : 0.0f;
    const ImVec2 windowPadding = ImGui::GetCurrentWindow()->WindowPadding;

    ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + title_bar_vertical_offset));
    const ImVec2 title_bar_min = ImGui::GetCursorScreenPos();
    const ImVec2 title_bar_max = {ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f,
                                  ImGui::GetCursorScreenPos().y + title_bar_height};
    auto *fgDrawList = ImGui::GetForegroundDrawList();
    fgDrawList->AddRectFilled(title_bar_min, title_bar_max, ImColor(0.0f, 0.0f, 0.0f, 0.2f));
    fgDrawList->AddLine(ImVec2(title_bar_min.x, title_bar_max.y), title_bar_max, ImColor(0.7f, 0.7f, 0.7f, 0.3f), 1);
    {
      const auto text_size = ImGui::CalcTextSize(m_specification.name.c_str());
      fgDrawList->AddText(ImVec2(ImGui::GetWindowWidth() / 2 - text_size.x / 2, title_bar_height / 2 - text_size.y / 2),
                          UI::Colors::Theme::text_darker, m_specification.name.c_str());
    }

    ImGui::BeginHorizontal("Titlebar",
                           {ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetFrameHeightWithSpacing()});

    const float w = ImGui::GetContentRegionAvail().x;
    constexpr float buttonsAreaWidth = 102.0f;
    constexpr float leftButtonAreaWidth = 102.0f;

    ImGui::SetCursorPos(ImVec2(windowPadding.x + leftButtonAreaWidth, windowPadding.y + title_bar_vertical_offset));
    ImGui::InvisibleButton("##titleBarDragZone", ImVec2(w - buttonsAreaWidth - leftButtonAreaWidth, title_bar_height));

    m_titlebar_hovered = ImGui::IsItemHovered();

    if (isMaximized) {
      if (const float windowMousePosY = ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y;
          windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
        m_titlebar_hovered = true;
    }

    if (m_menubar_callback) {
      ImGui::SuspendLayout();
      {
        ImGui::SetItemAllowOverlap();
        const float logoHorizontalOffset = 16.0f * 2.0f + 48.0f + windowPadding.x;
        ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, 6.0f + title_bar_vertical_offset));
        DrawMenubar();

        if (ImGui::IsItemHovered()) m_titlebar_hovered = false;
      }

      ImGui::ResumeLayout();
    }

    const ImU32 buttonColN = UI::Colors::Theme::text;
    const ImU32 buttonColH = UI::Colors::Theme::text_darker;
    constexpr ImU32 buttonColP = UI::Colors::Theme::text_darker;
    constexpr float buttonWidth = 14.0f;
    constexpr float buttonHeight = 14.0f;


    ImGui::Spring();
    ShiftCursorY(8.0f);
    {
      const int iconHeight = static_cast<int>(m_icon_minimize->GetHeight());
      const float padY = (buttonHeight - static_cast<float>(iconHeight)) / 2.0f;
      if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight))) {
        if (const auto application = Get(); application.has_value()) {
          (*application)->QueueEvent([windowHandle = m_window]() { glfwIconifyWindow(windowHandle); });
        }
      }

      DrawButtonImage(m_icon_minimize, buttonColN, buttonColH, buttonColP, RectExpanded(GetItemRect(), 0.0f, -padY));
    }

    ImGui::Spring(-1.0f, 17.0f);
    ShiftCursorY(8.0f);
    {
      const bool isMaximized = IsMaximized();

      if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight))) {
        if (const auto application = Get(); application.has_value()) {
          (*application)->QueueEvent([isMaximized, windowHandle = m_window]() {
            if (isMaximized)
              glfwRestoreWindow(windowHandle);
            else
              glfwMaximizeWindow(windowHandle);
          });
        }
      }

      DrawButtonImage(isMaximized ? m_icon_restore : m_icon_maximize, buttonColN, buttonColH, buttonColP);
    }

    ImGui::Spring(-1.0f, 15.0f);
    ShiftCursorY(8.0f);
    {
      const int iconHeight = static_cast<int>(m_icon_close->GetHeight());
      const float padY = (buttonHeight - static_cast<float>(iconHeight)) / 2.0f;

      if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight))) {
        if (const auto application = Get(); application.has_value()) {
          (*application)->Close();
        }
      }

      DrawButtonImage(m_icon_close, buttonColN, buttonColH, buttonColP, RectExpanded(GetItemRect(), 0.0f, padY));
    }

    ImGui::Spring(-1.0f, 18.0f);
    ImGui::EndHorizontal();

    out_title_bar_height = title_bar_height;
  }


  std::unique_ptr<Application> Application::CreateApplication(int argc, char **argv, std::unique_ptr<Layer> layer) {
    const auto specifications = ApplicationSpecifications{
        "Infinity Launcher",
        std::make_pair(1440, 1026),
        std::make_pair(3840, 2160),
        std::make_pair(1240, 680),
        true,
#ifdef WIN32
        true,
#else
        false,
#endif
    };
    auto app = std::make_unique<Application>(specifications);
    app->PushLayer(std::move(layer));

    return app;
  }


}  // namespace Infinity
