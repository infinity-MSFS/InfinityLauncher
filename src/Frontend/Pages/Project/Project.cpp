
#include "Project.hpp"

#include <imgui.h>
#include <utility>

#include "Backend/Image/Image.hpp"
#include "Frontend/Markdown/Markdown.hpp"
#include "Util/State/State.hpp"

namespace Infinity {

    std::shared_ptr<uint8_t> ProjectPage::m_SelectedAircraft = std::make_shared<uint8_t>(0);
    std::shared_ptr<uint8_t> ProjectPage::m_SelectedPage = std::make_shared<uint8_t>(0);

    ProjectPage::ProjectPage(const GroupData &group_data, GroupDataImages state_images) :
        m_GroupData(group_data), m_StateImages(std::move(state_images)), m_ContentRegion(&m_GroupData, m_SelectedPage), m_TopRegion(&m_GroupData, m_SelectedAircraft) {}


    void ProjectPage::Render() {
        auto &state = State::GetInstance();
        if (const auto main_state = state.GetPageState<MainState>("main"); main_state.has_value()) {
            ImGui::Text("Project");
            ImGui::Separator();
            ImGui::Text("%s", m_GroupData.name.c_str());
            if (!m_StateImages.projectImages.empty()) {
#ifdef WIN32
                constexpr float top_padding = 45.0f;
#else
                constexpr float top_padding = 0.0f;
#endif
                if (m_StateImages.projectImages[0].pageBackgroundImage.has_value()) {

                    // TODO: Implement Image::RenderProjectImage
                    ImGui::Text("Project Image");
                    Image::RenderHomeImage(m_StateImages.projectImages[0].pageBackgroundImage.value(), {0.0f, top_padding}, {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()}, true);
                    m_TopRegion.Render();
                    m_ContentRegion.Render();
                } else if (m_StateImages.projectImages[0].backgroundImage) {
                    ImGui::Text("Project Image");
                    Image::RenderHomeImage(m_StateImages.projectImages[0].backgroundImage, {0.0f, top_padding}, {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()}, true);
                    m_TopRegion.Render();
                    m_ContentRegion.Render();
                }
            }
        }
    }


    AircraftSelectButton::AircraftSelectButton(std::string name, int32_t id, const std::shared_ptr<uint8_t> &selected_aircraft) :
        m_Name(std::move(name)), m_Id(id), m_Active(false), m_SelectedAircraft(selected_aircraft) {}

    bool AircraftSelectButton::IsActive() { return *m_SelectedAircraft == m_Id; }


    void AircraftSelectButton::Render() {
        // TODO: Fancy button
        if (IsActive()) {
            ImGui::Text("Active");
        }
        if (ImGui::Button(m_Name.c_str())) {
            *m_SelectedAircraft = static_cast<uint8_t>(m_Id);
        }
    }

    AircraftSelectButtonBar::AircraftSelectButtonBar(const std::vector<AircraftSelectButton> &aircraft_select_buttons, const std::shared_ptr<uint8_t> &selected_aircraft) :
        m_AircraftSelectButtons(std::move(aircraft_select_buttons)), m_SelectedAircraft(selected_aircraft) {}

    void AircraftSelectButtonBar::Render() {
        for (auto &button: m_AircraftSelectButtons) {
            button.Render();
        }
    }

    TopRegion::TopRegion(GroupData *group_data, const std::shared_ptr<uint8_t> &selected_aircraft) : m_AircraftSelectButtonBar({}, selected_aircraft), m_SelectedAircraft(selected_aircraft) {
        for (int32_t i = 0; i < group_data->projects.size(); i++) {
            m_AircraftSelectButtons.emplace_back(AircraftSelectButton(group_data->projects[i].name, i, selected_aircraft));
        }
        m_AircraftSelectButtonBar = AircraftSelectButtonBar(m_AircraftSelectButtons, selected_aircraft);
    }

    void TopRegion::Render() { m_AircraftSelectButtonBar.Render(); }


    ContentRegionButton::ContentRegionButton(const std::string &name, int8_t id, const std::shared_ptr<uint8_t> &selected_page) : m_ButtonSpec(name, id), m_SelectedPage(selected_page) {}

    ContentRegionButton::ContentRegionButton(const Button &button, const std::shared_ptr<uint8_t> &selected_page) : m_ButtonSpec(button), m_SelectedPage(selected_page) {}


    void ContentRegionButton::Render() {
        if (*m_SelectedPage == m_ButtonSpec.page) {
            ImGui::Text("Active");
        }
        if (ImGui::Button(m_ButtonSpec.name.c_str())) {
            std::cout << "Selected page: " << m_ButtonSpec.page << std::endl;
            *m_SelectedPage = m_ButtonSpec.page;
        }
    }

    ContentRegionButtonBar::ContentRegionButtonBar(std::vector<ContentRegionButton> buttons, const std::shared_ptr<uint8_t> &selected_page) :
        m_Buttons(std::move(buttons)), m_SelectedPage(selected_page) {}

    void ContentRegionButtonBar::Render() {
        for (auto &button: m_Buttons) {
            button.Render();
        }
    }

    ContentRegion::ContentRegion(GroupData *group_data, const std::shared_ptr<uint8_t> &selected_page) : m_GroupData(group_data), m_SelectedPage(selected_page), m_ButtonBar({}, selected_page) {
        auto overview_button = ContentRegionButton("Overview", 0, m_SelectedPage);
        auto description_button = ContentRegionButton("Description", 1, m_SelectedPage);
        auto changelog_button = ContentRegionButton("Changelog", 2, m_SelectedPage);
        m_ButtonBar = ContentRegionButtonBar({overview_button, description_button, changelog_button}, m_SelectedPage);
    }
    void ContentRegion::Render() {


        ImGui::Text("Content Region");
        ImGui::Separator();
        m_ButtonBar.Render();
        ImGui::Text("Selected page: %d", *m_SelectedPage);
        ImGui::Text("Description: %s", m_Description.c_str());
        ImGui::Text("Overview: %s", m_Overview.c_str());

        for (auto &changelog: m_Changelogs) {
            ImGui::Text("Change log: %s", changelog.c_str());
        }
    }


} // namespace Infinity
