
#include "Project.hpp"

#include <imgui.h>
#include <utility>

#include "Backend/Image/Image.hpp"
#include "Frontend/Markdown/Markdown.hpp"
#include "Util/State/State.hpp"

namespace Infinity {
    ProjectPage::ProjectPage(GroupData group_data, GroupDataImages state_images) : m_GroupData(std::move(group_data)), m_StateImages(std::move(state_images)) {}


    void ProjectPage::Render() {
        auto &state = State::GetInstance();
        if (const auto main_state = state.GetPageState<MainState>("main"); main_state.has_value()) {
            ImGui::Text("Project");
            ImGui::Separator();
            ImGui::Text("%s", m_GroupData.name.c_str());
            if (!m_StateImages.projectImages.empty()) {
                if (m_StateImages.projectImages[0].pageBackgroundImage.has_value()) {
#ifdef WIN32
                    constexpr float top_padding = 45.0f;
#else
                    constexpr float top_padding = 0.0f;
#endif
                    Image::RenderHomeImage(m_StateImages.projectImages[0].pageBackgroundImage.value(), {0.0f, top_padding}, {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()}, true);

                    auto md = Markdown::GetInstance();

                    const std::string markdownText = std::string(reinterpret_cast<const char *>(u8R"(
# H1 Header: Text and Links
This is a [link to github](https://github.com/infinity-MSFS).
This is an image that you cannot see: ![image](this_should_be_removed_anyways)
Horizontal rules:
***
___
*Emphasis* and **strong emphasis**.
## H2 Header: indented text.
  This text has an indent (two leading spaces).
    This one has two.
### H3 Header: Lists
  - Unordered list
    - List with indent.
      - List with [link](https://google.com) and *emphasized text*
)"));
                    md->Render(markdownText);
                }
            }
        }
    }
} // namespace Infinity
