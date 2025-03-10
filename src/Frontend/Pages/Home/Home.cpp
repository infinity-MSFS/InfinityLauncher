
#include "Home.hpp"

#include "Backend/Application/Application.hpp"
#include "Backend/Router/Router.hpp"
#include "Frontend/SVG/SVGDrawing.hpp"
#include "imgui.h"

#include <numeric>

#include "Frontend/Pages/Project/Project.hpp"

namespace Infinity {

    unsigned int Home::m_ExpectedProjects = 800000;
    bool Home::m_DoneLoading = false;

    void Home::Render() const {
        ImGui::PushFont(Application::GetFont("DefaultLarge"));
        auto text_size = ImGui::CalcTextSize("Infinity");
        std::vector<unsigned int> active_index(242 - 1);
        std::iota(active_index.begin(), active_index.end(), 1);
        // TODO: Replace with a png resource
        DrawInfinityLogoHome(0.09f, ImVec2(ImGui::GetWindowWidth() / 2 - 50.0f, 70.0f), active_index, 2.0f);
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - text_size.x / 2 + 50, 80.0f));
        ImGui::Text("Infinity");
        ImGui::PopFont();
        size_t index = 3; // skip reserved pages
        for (const auto &project: m_HomeProjectButtons) {
            RenderProject(project, index);
            index++;
        }
    }

    void Home::RegisterProject(const std::string &name, std::shared_ptr<Image> image, std::shared_ptr<Image> logo, const int page_id) { m_HomeProjectButtons.emplace_back(name, image, logo, page_id); }

    void Home::RegisterProject(const std::vector<HomeProjectButtonStruct> &projects) {
        // for (const auto &project: projects) {
        // }
        // m_HomeProjectButtons = projects;
    }

    void Home::UnregisterProject(const std::string &name) {
        for (int i = 0; i < m_HomeProjectButtons.size(); i++) {
            if (m_HomeProjectButtons[i].name == name) {
                m_HomeProjectButtons.erase(m_HomeProjectButtons.begin() + i);
            }
        }
    }

    void Home::UnregisterProject(const std::vector<std::string> &projects) {
        for (const auto &project: projects) {
            UnregisterProject(project);
        }
    }

    void Home::RenderProject(const HomeProjectButtonStruct &project, const int page_index) {
        const float base_x = ImGui::GetWindowWidth() / 4;
        const float x_pos = base_x - base_x + 30.0f + static_cast<float>(page_index - 3) * base_x;
        constexpr float y_pos = 150.0f;
        const ImVec2 position(x_pos, y_pos);
        const ImVec2 size(ImGui::GetWindowWidth() / 4 - 50.0f, ImGui::GetWindowHeight() - 250.0f);

        // TODO: Max logo size
        const ImVec2 logo_size(ImGui::GetWindowWidth() / 4 - 150.0f, ImGui::GetWindowWidth() / 4 - 150.0f);
        const auto logo_position = ImVec2(x_pos + size.x / 2 - logo_size.x / 2, ImGui::GetWindowHeight() - logo_size.y - 150.0f);

        ImGui::SetCursorPos(position);

        const bool clicked = ImGui::InvisibleButton(project.name.c_str(), size);

        const bool is_hovered = ImGui::IsItemHovered();

        Image::RenderHomeImage(project.image, position, size, is_hovered);
        Image::RenderImage(project.logo, logo_position, logo_size);

        if (clicked) {
            if (const auto router = Utils::Router::getInstance(); router.has_value()) {
                ProjectPage::ResetState();
                if (auto result = (*router)->setPage(page_index); !result.has_value()) {
                    Errors::Error(result.error()).Dispatch();
                }
            }
        }
        ImGui::PushFont(Application::GetFont("DefaultLarge"));
        const auto text_size = ImGui::CalcTextSize(project.name.c_str());
        ImGui::SetCursorPos({x_pos + size.x / 2 - text_size.x / 2, ImGui::GetWindowHeight() - 80.0f});
        ImGui::Text("%s", project.name.c_str());
        ImGui::PopFont();
    }

    Home *Home::GetInstance() {
        static Home instance;
        return &instance;
    }


} // namespace Infinity
