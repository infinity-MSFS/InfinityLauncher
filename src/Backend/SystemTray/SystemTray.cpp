#include "SystemTray.hpp"

#include <thread>

#include "Backend/Application/Application.hpp"
#include "Backend/Router/Router.hpp"

SystemTray::SystemTray(GLFWwindow *glf_window)
    : m_tray({"Infinity Launcher", "Assets/Images/Logo.png"})
    , m_GlfwWindow(glf_window) {}

void SystemTray::handle_close() {
  if (const auto application = Infinity::Application::Get(); application.has_value()) {
    (*application)->Close();
  }
}

void SystemTray::handle_show_downloads() {
  if (const auto router = Infinity::Utils::Router::getInstance(); router.has_value()) {
    router.value()->setPage(2);
  }
}

void SystemTray::handle_show_home() {
  if (const auto router = Infinity::Utils::Router::getInstance(); router.has_value()) {
    router.value()->setPage(0);
  }
}

void SystemTray::handle_show_settings() {
  if (const auto router = Infinity::Utils::Router::getInstance(); router.has_value()) {
    router.value()->setPage(1);
  }
}

void SystemTray::run() {
  m_tray.addEntry(Tray::Button("Home", [this] { handle_show_home(); }));
  m_tray.addEntry(Tray::Button("Downloads", [this] { handle_show_downloads(); }));
  m_tray.addEntry(Tray::Button("Settings", [this] { handle_show_settings(); }));
  m_tray.addEntry(Tray::Button("Close", [this] { handle_close(); }));


  end_thread();

  m_should_exit.store(false);

  m_tray_thread = std::make_unique<std::thread>([this] {
    while (!m_should_exit.load()) {
      m_tray.run();
    }
  });
  m_tray_thread->detach();
}

void SystemTray::end_thread() {
  if (m_tray_thread) {
    m_should_exit.store(true);
    m_tray_thread.reset();
  }
}
