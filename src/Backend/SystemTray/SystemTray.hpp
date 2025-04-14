#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <tray.hpp>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

class SystemTray {
  public:
  explicit SystemTray(GLFWwindow *glf_window);

  void run();

  void end_thread();

  private:
  void handle_close();
  void handle_show_downloads();
  void handle_show_settings();
  void handle_show_home();


  std::string m_icon_path;
  Tray::Tray m_tray;
  std::unique_ptr<std::thread> m_tray_thread;
  std::atomic<bool> m_should_exit{false};

  GLFWwindow *m_GlfwWindow;
};
