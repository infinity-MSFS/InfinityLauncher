
#include "Home.hpp"

#include "imgui.h"
#include "Backend/Router/Router.hpp"

namespace Infinity {
    void Home::Render() {
        size_t index = 1;
        for (const auto project: m_HomeProjectButtons) {
            RenderProject(project, index);
            index++;
        }
    }

    void Home::RegisterProject(const std::string &name, const std::string &image_link, const std::string &logo_link, int page_id) {
        m_HomeProjectButtons.push_back({name, Image::LoadFromURL(image_link), Image::LoadFromURL(logo_link), page_id});
    }

    void Home::RegisterProject(const std::vector<HomeProjectButtonStruct> &projects) {
        m_HomeProjectButtons = projects;
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

    void Home::RenderProject(const HomeProjectButtonStruct &project, int page_index) {
        float base_x = ImGui::GetWindowWidth() / 4;
        float x_pos = base_x - base_x + 30.0f + static_cast<float>(page_index - 1) * base_x;
        float y_pos = 50.0f;
        ImVec2 position(x_pos, y_pos);
        ImVec2 size(ImGui::GetWindowWidth() / 4 - 50.0f, ImGui::GetWindowHeight() - 150.0f);


        ImVec2 logo_size(ImGui::GetWindowWidth() / 4 - 150.0f, ImGui::GetWindowWidth() / 4 - 150.0f);
        ImVec2 logo_position = ImVec2(x_pos + size.x / 2 - logo_size.x / 2, ImGui::GetWindowHeight() - logo_size.y - 150.0f);

        ImGui::SetCursorPos(position);

        ImGui::SetCursorPos(position);
        bool clicked = ImGui::InvisibleButton(project.name.c_str(), size);

        bool is_hovered = ImGui::IsItemHovered();

        Image::RenderHomeImage(project.image, position, size, is_hovered);
        Image::RenderImage(project.logo, logo_position, logo_size);

        if (clicked) {
            auto router = Utils::Router::getInstance();
            if (router.has_value()) {
                (*router)->setPage(page_index);
            }
        }
        ImGui::PushFont(Application::Get().value()->GetFont("DefaultLarge"));
        auto text_size = ImGui::CalcTextSize(project.name.c_str());
        ImGui::SetCursorPos({x_pos + size.x / 2 - text_size.x / 2, ImGui::GetWindowHeight() - 80.0f});
        ImGui::Text("%s", project.name.c_str());
        ImGui::PopFont();

    }

    Home *Home::GetInstance() {
        static Home instance;
        return &instance;
    }


}
