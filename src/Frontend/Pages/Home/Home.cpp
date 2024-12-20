
#include "Home.hpp"

#include "imgui.h"

namespace Infinity {
    void Home::Render() {
        ImGui::Text("Home");
    }

    void Home::RegisterProject(const std::string &name, const std::string &image_link, int page_id) {
        m_HomeProjectButtons.push_back({name, image_link, page_id});
    }

    void Home::RegisterProject(const std::vector<HomeProjectButtonStruct> &projects) {
        for (const auto &project: projects) {
            m_HomeProjectButtons.push_back(project);
        }
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


}
