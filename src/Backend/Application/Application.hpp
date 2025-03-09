#pragma once

#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>


#include <GLFW/glfw3.h>
#include "Backend/Image/Image.hpp"
#include "Backend/Layer/Layer.hpp"
#include "Util/Error/Error.hpp"

#include "imgui.h"

namespace Infinity {

    struct ApplicationSpecifications {
        std::string name;
        std::pair<uint32_t, uint32_t> window_size;
        std::pair<uint32_t, uint32_t> max_size;
        std::pair<uint32_t, uint32_t> min_size;
        bool resizable;
        bool custom_titlebar;
    };

    class Application {
    public:
        explicit Application(const ApplicationSpecifications &specifications);
        ~Application();

        static std::unique_ptr<Application> CreateApplication(int argc, char **argv, std::unique_ptr<Layer> layer);

        template<typename T>
        void PushLayer() {
            static_assert(std::is_base_of_v<Layer, T>, "T must derive from Layer");
            m_Layer = std::make_shared<T>();
            m_Layer->OnAttach();
        }

        void PushLayer(const std::shared_ptr<Layer> &layer) {
            m_Layer = layer;
            layer->OnAttach();
        }

        std::expected<void, Errors::Error> Run();

        void Close();

        [[nodiscard]] bool IsMaximized() const;

        [[nodiscard]] std::shared_ptr<Image> GetIcon() const { return m_AppHeaderIcon; }

        [[nodiscard]] static float GetTime();

        static ImFont *GetFont(const std::string &name);

        template<typename F>
        void QueueEvent(F &&func) {
            m_EventQueue.push(func);
        }

        static void SetWindowTitle(const std::string &title);

        static std::optional<Application *> Get();

        ApplicationSpecifications GetSpecifications() { return m_Specification; }

    private:
        std::expected<void, Errors::Error> Init();
        const char *SetupGLVersion();
        std::expected<void, Errors::Error> Shutdown();
        static void SetWindowIcon(GLFWwindow *window, const unsigned char *data, int size);

        void DrawTitleBar(float &out_title_bar_height);
        void DrawMenubar() const;

        static void GLFWErrorCallback(int error, const char *description);

    private:
        ApplicationSpecifications m_Specification;
        static Application *s_Instance;
        GLFWwindow *m_Window;

        std::unordered_map<std::string, ImFont *> m_Fonts;

        bool m_Running = true;

        float m_TimeStep = 0.0f;
        float m_FrameTime = 0.0f;
        float m_LastFrameTime = 0.0f;

        bool m_TitlebarHovered = false;

        std::shared_ptr<Layer> m_Layer;
        std::function<void()> m_MenubarCallback;

        std::mutex m_EventQueueMutex;
        std::queue<std::function<void()>> m_EventQueue;

        std::shared_ptr<Image> m_AppHeaderIcon;
        std::shared_ptr<Image> m_IconClose;
        std::shared_ptr<Image> m_IconMinimize;
        std::shared_ptr<Image> m_IconMaximize;
        std::shared_ptr<Image> m_IconRestore;
    };


} // namespace Infinity
