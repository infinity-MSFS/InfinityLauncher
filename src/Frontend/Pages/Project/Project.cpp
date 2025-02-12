
#include "Project.hpp"

#include <imgui.h>
#include <utility>

#include "Backend/Image/Image.hpp"

namespace Infinity {
    ProjectPage::ProjectPage(GroupData group_data): m_GroupData(std::move(group_data)) {
        m_Image = Image::LoadFromURL(m_GroupData.projects[0].background);
    }


    void ProjectPage::Render() {
        ImGui::Text("Project");
        ImGui::Separator();
        ImGui::Text("%s", m_GroupData.name.c_str());
        Image::RenderHomeImage(m_Image, {50.0f, 50.0f}, {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()}, true);
    }
}
