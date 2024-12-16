
#include "Project.hpp"

#include <imgui.h>
#include <utility>

namespace Infinity {
    ProjectPage::ProjectPage(GroupData group_data):
        m_GroupData(std::move(group_data)) {
    }


    void ProjectPage::Render() {
        ImGui::Text("Project");
    }


}
