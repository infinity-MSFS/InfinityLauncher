
#include "Project.hpp"

#include <imgui.h>
#include <utility>

#include "Util/State/State.hpp"
#include "Backend/Image/Image.hpp"

namespace Infinity {
    ProjectPage::ProjectPage(GroupData group_data, GroupDataImages state_images): m_GroupData(std::move(group_data)),
        m_StateImages(std::move(state_images)) {
    }


    void ProjectPage::Render() {
        auto &state = State::GetInstance();
        if (const auto main_state = state.GetPageState<MainState>("main");
            main_state.has_value()) {
            ImGui::Text("Project");
            ImGui::Separator();
            ImGui::Text("%s", m_GroupData.name.c_str());
            if (m_StateImages.projectImages.size() > 0) {
                if (m_StateImages.projectImages[0].pageBackgroundImage.has_value()) {
                    Image::RenderHomeImage(m_StateImages.projectImages[0].pageBackgroundImage.value(), {0.0f, 45.0f},
                                           {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()}, true);
                }
            }
        }
    }
}
