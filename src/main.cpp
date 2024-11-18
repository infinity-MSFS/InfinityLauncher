#include <iostream>
#include <filesystem>

#include "Backend/Application/Application.hpp"

bool g_ApplicationRunning = true;

class PageRenderLayer final : public Infinity::Layer {
public:
    void OnUIRender() override {
        ImGui::Text("The Monkeys will win the war");
    }
};

Infinity::Application *Infinity::CreateApplication(int argc, char **argv) {
    const std::filesystem::path path = "Assets/Images/Logo.h";
    const ApplicationSpecifications specifications = ApplicationSpecifications{
        "Infinity Launcher",
        std::make_pair(1440, 1026),
        std::make_pair(2560, 1440),
        std::make_pair(1240, 680),
        path,
        true,
#ifdef WIN32
        true,
#else
        false,
#endif
        true
    };
    const auto app = new Application{specifications};
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
}


#if defined(RELEASE_DIST) && WIN32

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nShowCmd) { Infinity::Main(__argc, __argv); }

#else

int main(const int argc, char **argv) { Infinity::Main(argc, argv); }

#endif
