#pragma once

#include <GLFW/glfw3.h>
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

#include "Backend/Image/Image.hpp"
#include "Backend/Layer/Layer.hpp"
#include "GL/glew.h"
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
      m_layer = std::make_shared<T>();
      m_layer->OnAttach();
    }

    void PushLayer(const std::shared_ptr<Layer> &layer) {
      m_layer = layer;
      layer->OnAttach();
    }

    std::expected<void, Errors::Error> Run();

    void Close();

    [[nodiscard]] std::shared_ptr<Image> GetAppLogo() { return m_app_header_icon; }

    [[nodiscard]] bool IsMaximized() const;

    [[nodiscard]] std::shared_ptr<Image> GetIcon() const { return m_app_header_icon; }

    [[nodiscard]] static float GetTime();

    static ImFont *GetFont(const std::string &name);

    template<typename F>
    void QueueEvent(F &&func) {
      m_event_queue.push(func);
    }

    static void SetWindowTitle(const std::string &title);

    static std::optional<Application *> Get();

    ApplicationSpecifications GetSpecifications() { return m_specification; }

    [[nodiscard]] bool IsTitleBarHovered() const { return m_titlebar_hovered; }


    [[nodiscard]] unsigned int GetFPS() const { return m_fps_cap; }
    [[nodiscard]] bool IsReduceFPSOnIdle() const { return m_reduce_fps_on_idle; }

    void SetFPS(const unsigned int fps) {
      if (fps > 0) {
        m_fps_cap = fps;
      }
    }

    void SetReduceFPSOnIdle(const bool reduce) { m_reduce_fps_on_idle = reduce; }

private:
    std::expected<void, Errors::Error> Init();
    static const char *SetupGLVersion();
    std::expected<void, Errors::Error> Shutdown();
    static void SetWindowIcon(GLFWwindow *window, const unsigned char *data, int size);

    void DrawTitleBar(float &out_title_bar_height);
    void DrawMenubar() const;

    static void GLFWErrorCallback(int error, const char *description);

    static void ProcessImageQueue();

    void CapFPS(double &last_frame_time) const;


private:
    ApplicationSpecifications m_specification;
    static Application *s_instance;
    GLFWwindow *m_window;

    std::unordered_map<std::string, ImFont *> m_Fonts;

    bool m_running = true;

    unsigned int m_fps_cap = 144;
    bool m_reduce_fps_on_idle = true;

    float m_time_step = 0.0f;
    float m_frame_time = 0.0f;
    float m_last_frame_time = 0.0f;

    bool m_titlebar_hovered = false;

    std::shared_ptr<Layer> m_layer;
    std::function<void()> m_menubar_callback;

    std::mutex m_event_queue_mutex;
    std::queue<std::function<void()>> m_event_queue;

    struct ImageStore {
      GLuint texture_id;
      int width;
      int height;
    };

    std::shared_ptr<Image> m_app_header_icon;
    std::shared_ptr<Image> m_icon_close;
    std::shared_ptr<Image> m_icon_minimize;
    std::shared_ptr<Image> m_icon_maximize;
    std::shared_ptr<Image> m_icon_restore;

    std::vector<std::shared_ptr<Image>> m_texture_creation_queue;
    std::mutex m_texture_creation_queue_mutex;
  };


}  // namespace Infinity
