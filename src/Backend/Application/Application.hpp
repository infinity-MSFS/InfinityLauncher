
#pragma once


#include "Backend/Layer/Layer.hpp"
#include "Backend/Image/Image.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <vector>
#include <utility>


#include "imgui.h"
#include "vulkan/vulkan.h"


void check_vk_result(VkResult err);

struct GLFWwindow;

namespace Infinity {
    struct ApplicationSpecifications {
        std::string name;
        std::pair<uint32_t, uint32_t> window_size;
        std::pair<uint32_t, uint32_t> max_size;
        std::pair<uint32_t, uint32_t> min_size;
        std::filesystem::path icon_path;
        bool resizable;
        bool custom_titlebar;
        bool center_window;
    };

    class Application {
    public:
        explicit Application(ApplicationSpecifications applicationSpecification);

        ~Application();

        static std::optional<Application *> Get();

        ApplicationSpecifications GetSpecifications() { return m_Specification; }

        void Run();

        void SetMenubarCallback(const std::function<void()> &menubarCallback) { m_MenubarCallback = menubarCallback; }

        template<typename T>
        void PushLayer() {
            static_assert(std::is_base_of_v<Layer, T>, "Pushed type is not subclass of Layer!");
            m_LayerStack.emplace_back(std::make_shared<T>())->OnAttach();
        }

        void PushLayer(const std::shared_ptr<Layer> &layer) {
            m_LayerStack.emplace_back(layer);
            layer->OnAttach();
        }

        void Close();

        [[nodiscard]] bool IsMaximized() const;

        [[nodiscard]] std::shared_ptr<Image> GetApplicationIcon() const { return m_AppHeaderIcon; }

        static float GetTime();

        [[nodiscard]] GLFWwindow *GetWindowHandle() const { return m_WindowHandle; }
        [[nodiscard]] bool IsTitleBarHovered() const { return m_TitleBarHovered; }

        static std::optional<VkInstance> GetInstance();

        static std::optional<VkPhysicalDevice> GetPhysicalDevice();

        static std::optional<VkDevice> GetDevice();

        static VkCommandBuffer GetCommandBuffer(bool begin);

        static void FlushCommandBuffer(VkCommandBuffer commandBuffer);

        static void SubmitResourceFree(std::function<void()> &&func);

        static ImFont *GetFont(const std::string &name);

        template<typename Func>
        void QueueEvent(Func &&func) {
            m_EventQueue.push(func);
        }


        static void SetWindowTitle(const std::string &title);


        static GLFWwindow *s_WindowHandle;

    private:
        void Init();

        void Shutdown();

        static void SetWindowIcon(GLFWwindow *window, const unsigned char *data, int size);

        void UI_DrawTitleBar(float &out_title_bar_height);

        void UI_DrawMenubar() const;


    private:
        ApplicationSpecifications m_Specification;
        GLFWwindow *m_WindowHandle = nullptr;

        bool m_Running = false;

        float m_TimeStep = 0.0f;
        float m_FrameTime = 0.0f;
        float m_LastFrameTime = 0.0f;

        bool m_TitleBarHovered = false;

        std::vector<std::shared_ptr<Layer>> m_LayerStack;
        std::function<void()> m_MenubarCallback;

        std::mutex m_EventQueueMutex;
        std::queue<std::function<void()>> m_EventQueue;


        std::shared_ptr<Image> m_AppHeaderIcon;
        std::shared_ptr<Image> m_IconClose;
        std::shared_ptr<Image> m_IconMinimize;
        std::shared_ptr<Image> m_IconMaximize;
        std::shared_ptr<Image> m_IconRestore;
    };

    Application *CreateApplication(int argc, char **argv);
}