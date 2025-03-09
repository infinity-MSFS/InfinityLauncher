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
#include "Backend/SystemTray/SystemTray.hpp"
#include "Backend/UIHelpers/UiHelpers.hpp"
#include "Frontend/Theme/Theme.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"

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
#include "stb_image/stb_image.h"


#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif


constexpr int FPS_CAP = 144;
constexpr double FRAME_DURATION = 1.0 / FPS_CAP;

namespace Infinity {
    Application *Application::s_Instance = nullptr;

    Application::Application(const ApplicationSpecifications &specifications) : m_Specification(specifications), m_Window(nullptr) {
        if (auto result = Init(); !result.has_value()) {
            result.error().Dispatch();
        }
        s_Instance = this;
    }

    Application::~Application() { Shutdown(); }

    std::optional<Application *> Application::Get() {
        if (s_Instance == nullptr) {
            return std::nullopt;
        }
        return s_Instance;
    }

    void Application::GLFWErrorCallback(int error, const char *description) { std::cerr << "GLFW Error " << error << ": " << description << "\n"; }


    std::expected<void, Errors::Error> Application::Init() {

        std::cout << "Initializing" << std::endl;

        glfwSetErrorCallback(GLFWErrorCallback);

        if (!glfwInit()) {
            return std::unexpected(Errors::Error(Errors::ErrorType::Fatal, "Failed to initialize GLFW"));
        }

        const auto version = SetupGLVersion();

        m_Window = glfwCreateWindow(m_Specification.window_size.first, m_Specification.window_size.second, m_Specification.name.c_str(), nullptr, nullptr);

        if (m_Window == nullptr) {
            return std::unexpected(Errors::Error(Errors::ErrorType::Fatal, "Failed to create window"));
        }


        glfwMakeContextCurrent(m_Window);
        glfwSwapInterval(1);

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

        ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
        ImGui_ImplOpenGL3_Init(version);

        ImFontConfig font_config;
        font_config.FontDataOwnedByAtlas = false;
        ImFont *roboto = io.Fonts->AddFontFromMemoryTTF(g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &font_config);
        m_Fonts["Default"] = roboto;
        m_Fonts["Bold"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoBold, sizeof(g_RobotoBold), 20.0f, &font_config);
        m_Fonts["Italic"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoItalic, sizeof(g_RobotoItalic), 20.0f, &font_config);
        m_Fonts["DefaultLarge"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoRegular, sizeof(g_RobotoRegular), 32.0f, &font_config);
        m_Fonts["h1"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoBold, sizeof(g_RobotoBold), 32.0f, &font_config);
        m_Fonts["h2"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoBold, sizeof(g_RobotoBold), 24.0f, &font_config);
        m_Fonts["h3"] = io.Fonts->AddFontFromMemoryTTF(g_RobotoBold, sizeof(g_RobotoBold), 20.0f, &font_config);

        io.FontDefault = roboto;

        // TODO: load images

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
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
        // GL 3.0 + GLSL 130
        const char *glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

        return glsl_version;
    }

    std::expected<void, Errors::Error> Application::Shutdown() {
        std::cout << "Shutting down" << std::endl;
        m_Layer->OnDetach();
        m_AppHeaderIcon.reset();
        m_IconClose.reset();
        m_IconMinimize.reset();
        m_IconMaximize.reset();
        m_IconRestore.reset();


        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();


        glfwDestroyWindow(m_Window);
        glfwTerminate();

        m_Running = false;

        return {};
    }

    std::expected<void, Errors::Error> Application::Run() {
        m_Running = true;


        SystemTray system_tray(m_Window);

        const auto clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        ImGuiIO &io = ImGui::GetIO();

        io.IniFilename = nullptr;

        system_tray.run();

        while (!glfwWindowShouldClose(m_Window) && m_Running) {
            const double start_time = glfwGetTime();
            glfwPollEvents();
            {
                std::scoped_lock lock(m_EventQueueMutex);

                while (!m_EventQueue.empty()) {
                    auto &func = m_EventQueue.front();
                    func();
                    m_EventQueue.pop();
                }
            }
            m_Layer->OnUpdate(m_TimeStep);

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
                window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;
                window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
                if (!m_Specification.custom_titlebar && m_MenubarCallback)
                    window_flags |= ImGuiWindowFlags_MenuBar;


                ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
                if (m_Specification.custom_titlebar) {
                    float titleBarHeight;
                    DrawTitleBar(titleBarHeight);
                }

                m_Layer->OnUIRender();

                ImGui::PopStyleVar(3);

                ImGui::End();
            }

            ImGui::Render();

            int display_w, display_h;
            glfwGetFramebufferSize(m_Window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(m_Window);

            const float time = GetTime();
            m_FrameTime = time - m_LastFrameTime;
#ifdef INFINITY_WINDOWS
            m_TimeStep = min(m_FrameTime, 0.0333f);
#else
            m_TimeStep = std::min(m_FrameTime, 0.0333f);
#endif
            m_LastFrameTime = time;

            const double endTime = glfwGetTime();

            if (const double frameTime = endTime - start_time; frameTime < FRAME_DURATION) {
                std::this_thread::sleep_for(std::chrono::duration<double>(FRAME_DURATION - frameTime));
            }
        }
        return {};
    }

    void Application::Close() { m_Running = false; }

    float Application::GetTime() { return static_cast<float>(glfwGetTime()); }

    bool Application::IsMaximized() const { return static_cast<bool>(glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED)); }

    ImFont *Application::GetFont(const std::string &name) {
        if (!s_Instance->m_Fonts.contains(name)) {
            return nullptr;
        }
        return s_Instance->m_Fonts.at(name);
    }

    void Application::SetWindowIcon(GLFWwindow *window, const unsigned char *data, int size) {

        GLFWimage images[1];
        images[0].pixels = stbi_load_from_memory(data, size, &images[0].width, &images[0].height, nullptr, 4);

        if (images[0].pixels == nullptr) {
            std::cerr << "Failed to load image" << std::endl;
            return;
        }

        glfwSetWindowIcon(s_Instance->m_Window, 1, images);
        stbi_image_free(images[0].pixels);
    }

    void Application::SetWindowTitle(const std::string &title) { glfwSetWindowTitle(s_Instance->m_Window, title.c_str()); }

    void Application::DrawMenubar() const {
        if (!m_MenubarCallback)
            return;

        if (m_Specification.custom_titlebar) {
            const ImRect menuBarRect = {ImGui::GetCursorPos(), {ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing()}};

            ImGui::BeginGroup();
            if (BeginMenubar(menuBarRect)) {
                m_MenubarCallback();
            }

            EndMenubar();
            ImGui::EndGroup();
        } else {
            if (ImGui::BeginMenuBar()) {
                m_MenubarCallback();
                ImGui::EndMenuBar();
            }
        }
    }

    void Application::DrawTitleBar(float &out_title_bar_height) {

        constexpr float title_bar_height = 40.0f;
        const bool isMaximized = IsMaximized();
        const float title_bar_vertical_offset = isMaximized ? -6.0f : 0.0f;
        const ImVec2 windowPadding = ImGui::GetCurrentWindow()->WindowPadding;

        ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + title_bar_vertical_offset));
        const ImVec2 title_bar_min = ImGui::GetCursorScreenPos();
        const ImVec2 title_bar_max = {ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetCursorScreenPos().y + title_bar_height};
        auto *bgDrawList = ImGui::GetBackgroundDrawList();
        auto *fgDrawList = ImGui::GetForegroundDrawList();
        bgDrawList->AddRectFilled(title_bar_min, title_bar_max, UI::Colors::Theme::title_bar);
        fgDrawList->AddRectFilled(title_bar_min, title_bar_max, ImColor(0.0f, 0.0f, 0.0f, 0.2f));
        fgDrawList->AddLine(ImVec2(title_bar_min.x, title_bar_max.y), title_bar_max, ImColor(0.7f, 0.7f, 0.7f, 0.3f), 1);
        {
            constexpr int logoWidth = 36;
            constexpr int logoHeight = 36;
            const ImVec2 logoOffset(16.0f + windowPadding.x, 5.0f + windowPadding.y + title_bar_vertical_offset);
            const ImVec2 logoRectStart = {ImGui::GetItemRectMin().x + logoOffset.x, ImGui::GetItemRectMin().y + logoOffset.y};
            const ImVec2 logoRectMax = {logoRectStart.x + logoWidth, logoRectStart.y + logoHeight};
            // fgDrawList->AddImage(m_AppHeaderIcon->GetDescriptorSet(), logoRectStart, logoRectMax);
        }
        {
            auto text_size = ImGui::CalcTextSize(m_Specification.name.c_str());
            fgDrawList->AddText(ImVec2(ImGui::GetWindowWidth() / 2 - text_size.x / 2, title_bar_height / 2 - text_size.y / 2), UI::Colors::Theme::text_darker, m_Specification.name.c_str());
        }

        ImGui::BeginHorizontal("Titlebar", {ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetFrameHeightWithSpacing()});

        const float w = ImGui::GetContentRegionAvail().x;
        constexpr float buttonsAreaWidth = 94.0f;
        constexpr float leftButtonAreaWidth = 68.0f;

        ImGui::SetCursorPos(ImVec2(windowPadding.x + leftButtonAreaWidth, windowPadding.y + title_bar_vertical_offset));
        ImGui::InvisibleButton("##titleBarDragZone", ImVec2(w - buttonsAreaWidth - leftButtonAreaWidth, title_bar_height));

        m_TitlebarHovered = ImGui::IsItemHovered();

        if (isMaximized) {
            if (const float windowMousePosY = ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y; windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
                m_TitlebarHovered = true;
        }

        if (m_MenubarCallback) {
            ImGui::SuspendLayout();
            {
                ImGui::SetItemAllowOverlap();
                const float logoHorizontalOffset = 16.0f * 2.0f + 48.0f + windowPadding.x;
                ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, 6.0f + title_bar_vertical_offset));
                DrawMenubar();

                if (ImGui::IsItemHovered())
                    m_TitlebarHovered = false;
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
            const int iconHeight = static_cast<int>(m_IconMinimize->GetHeight());
            const float padY = (buttonHeight - static_cast<float>(iconHeight)) / 2.0f;
            if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight))) {

                if (const auto application = Get(); application.has_value()) {
                    (*application)->QueueEvent([windowHandle = m_Window]() { glfwIconifyWindow(windowHandle); });
                }
            }

            DrawButtonImage(m_IconMinimize, buttonColN, buttonColH, buttonColP, RectExpanded(GetItemRect(), 0.0f, -padY));
        }

        ImGui::Spring(-1.0f, 17.0f);
        ShiftCursorY(8.0f);
        {
            const bool isMaximized = IsMaximized();

            if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight))) {
                if (const auto application = Get(); application.has_value()) {
                    (*application)->QueueEvent([isMaximized, windowHandle = m_Window]() {
                        if (isMaximized)
                            glfwRestoreWindow(windowHandle);
                        else
                            glfwMaximizeWindow(windowHandle);
                    });
                }
            }

            DrawButtonImage(isMaximized ? m_IconRestore : m_IconMaximize, buttonColN, buttonColH, buttonColP);
        }

        ImGui::Spring(-1.0f, 15.0f);
        ShiftCursorY(8.0f);
        {
            if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight))) {
                if (const auto application = Get(); application.has_value()) {
                    (*application)->Close();
                }
            }

            DrawButtonImage(m_IconClose, UI::Colors::Theme::text, UI::Colors::Theme::text_error, buttonColP);
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


} // namespace Infinity
