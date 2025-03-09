#include "SystemTray.hpp"
#include <thread>
#include "Backend/Application/Application.hpp"
#include "Backend/Router/Router.hpp"

SystemTray::SystemTray(GLFWwindow *glf_window) : m_Tray({"Infinity Launcher", "Assets/Images/Logo.png"}), m_GlfwWindow(glf_window) {}

void SystemTray::handle_close() {
    if (const auto application = Infinity::Application::Get(); application.has_value()) {
        (*application)->Close();
    }
}

void SystemTray::handle_show_downloads() {
    if (auto router = Infinity::Utils::Router::getInstance(); router.has_value()) {
        router.value()->setPage(2);
    }
}

void SystemTray::handle_show_home() {
    if (auto router = Infinity::Utils::Router::getInstance(); router.has_value()) {
        router.value()->setPage(0);
    }
}

void SystemTray::handle_show_settings() {
    if (auto router = Infinity::Utils::Router::getInstance(); router.has_value()) {
        router.value()->setPage(1);
    }
}

void SystemTray::run() {
    m_Tray.addEntry(Tray::Button("Home", [this] { handle_show_home(); }));
    m_Tray.addEntry(Tray::Button("Downloads", [this] { handle_show_downloads(); }));
    m_Tray.addEntry(Tray::Button("Settings", [this] { handle_show_settings(); }));
    m_Tray.addEntry(Tray::Button("Close", [this] { handle_close(); }));


    end_thread();

    m_ShouldExit.store(false);

    m_TrayThread = std::make_unique<std::thread>([this] {
        while (!m_ShouldExit.load()) {
            m_Tray.run();
        }
    });
    m_TrayThread->detach();
}

void SystemTray::end_thread() {
    if (m_TrayThread) {
        m_ShouldExit.store(true);
        m_TrayThread.reset();
    }
}
