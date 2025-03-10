
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
        m_GroupData(group_data), m_StateImages(std::move(state_images)), m_ContentRegion(&m_GroupData, m_SelectedPage), m_TopRegion(&m_GroupData, &m_StateImages, m_SelectedAircraft) {}


    void ProjectPage::Render() {
        auto &state = State::GetInstance();
        if (const auto main_state = state.GetPageState<MainState>("main"); main_state.has_value()) {
            if (!m_StateImages.projectImages.empty()) {
#ifdef WIN32
                constexpr float top_padding = 45.0f;
#else
                constexpr float top_padding = 0.0f;
#endif
                if (m_StateImages.projectImages[0].pageBackgroundImage.has_value()) {
                    Image::RenderImage(m_StateImages.projectImages[0].pageBackgroundImage.value(), {0.0f, top_padding}, {ImGui::GetWindowWidth(), ImGui::GetWindowHeight() / 3.0f}, 0.5f);
                    m_TopRegion.Render();
                    // m_ContentRegion.Render();
                } else if (m_StateImages.projectImages[0].backgroundImage) {
                    Image::RenderImage(m_StateImages.projectImages[0].backgroundImage, {0.0f, top_padding}, {ImGui::GetWindowWidth(), ImGui::GetWindowHeight() / 3.0f}, 0.5f);
                    m_TopRegion.Render();
                    // m_ContentRegion.Render();
                }
            }
        }
    }


    AircraftSelectButton::AircraftSelectButton(std::string name, int32_t id, const std::shared_ptr<uint8_t> &selected_aircraft) :
        m_Name(std::move(name)), m_Id(id), m_Active(false), m_SelectedAircraft(selected_aircraft) {}

    bool AircraftSelectButton::IsActive() { return *m_SelectedAircraft == m_Id; }


    void AircraftSelectButton::Render(ImVec2 size, ImVec2 pos) {
        ImGui::SetCursorPos(pos);
        if (Button(m_Name.c_str(), size, IsActive())) {
            *m_SelectedAircraft = static_cast<uint8_t>(m_Id);
        }
    }

    bool AircraftSelectButton::Button(const char *label, ImVec2 size, bool active) {
        auto cursor_pos = ImGui::GetCursorScreenPos();

        bool pressed = ImGui::InvisibleButton(label, size);

        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImVec4 bg_color, border_color, text_color;

        if (active) {
            bg_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            border_color = bg_color;
            text_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        } else {
            bg_color = ImVec4(0, 0, 0, 0);
            border_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            text_color = border_color;
        }
        const ImVec2 active_arrow_size(60.0f, 30.0f);
        const ImVec2 active_arrow_location(cursor_pos.x + size.x / 2 - active_arrow_size.x / 2, cursor_pos.y + size.y);


        if (active) {
            draw_list->AddRectFilled(cursor_pos, ImVec2(cursor_pos.x + size.x, cursor_pos.y + size.y), ImGui::GetColorU32(bg_color), 2.0f);
            draw_list->AddTriangleFilled(active_arrow_location, ImVec2(active_arrow_location.x + active_arrow_size.x, active_arrow_location.y),
                                         ImVec2(active_arrow_location.x + active_arrow_size.x / 2, active_arrow_location.y + active_arrow_size.y), ImGui::GetColorU32(bg_color));
        }
        if (!active)
            draw_list->AddRect(cursor_pos, ImVec2(cursor_pos.x + size.x, cursor_pos.y + size.y), ImGui::GetColorU32(border_color), 2.0f, 0, 2.0f);


        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImVec2 textPos = ImVec2(cursor_pos.x + (size.x - textSize.x) * 0.5f, cursor_pos.y + (size.y - textSize.y) * 0.5f);
        draw_list->AddText(textPos, ImGui::GetColorU32(text_color), label);

        return pressed;
    }

    AircraftSelectButtonBar::AircraftSelectButtonBar(const std::vector<AircraftSelectButton> &aircraft_select_buttons, const std::shared_ptr<uint8_t> &selected_aircraft) :
        m_AircraftSelectButtons(std::move(aircraft_select_buttons)), m_SelectedAircraft(selected_aircraft) {}

    void AircraftSelectButtonBar::Render() {
        const int button_count = m_AircraftSelectButtons.size();
        constexpr float padding = 40.0f; // padding on left and right side of screen
        constexpr float gap = 10.0f; // gap between buttons
        const float button_width = (ImGui::GetWindowWidth() - padding * 2 - gap * (button_count - 1)) / button_count;
        constexpr float button_height = 50.0f;
        const ImVec2 button_size(button_width, button_height);
        int index = 0;
        for (auto &button: m_AircraftSelectButtons) {
            const float button_pos_x = padding + index * (button_width + gap);
            const float button_pos_y = ImGui::GetWindowHeight() / 4;
            button.Render(button_size, ImVec2(button_pos_x, button_pos_y));
            index++;
        }
    }

    TopRegion::TopRegion(GroupData *group_data, GroupDataImages *group_data_images, const std::shared_ptr<uint8_t> &selected_aircraft) :
        m_GroupData(group_data), m_GroupDataImages(group_data_images), m_AircraftSelectButtonBar({}, selected_aircraft), m_SelectedAircraft(selected_aircraft) {
        for (int32_t i = 0; i < group_data->projects.size(); i++) {
            m_AircraftSelectButtons.emplace_back(AircraftSelectButton(group_data->projects[i].name, i, selected_aircraft));
        }
        m_AircraftSelectButtonBar = AircraftSelectButtonBar(m_AircraftSelectButtons, selected_aircraft);
    }

    void TopRegion::Render() {
        ImGui::PushFont(Application::GetFont("DefaultXLarge"));
        const float gap_text_logo = 10.0f;
        const ImVec2 logo_size(150.0f, 150.0f);
        const auto text_size = ImGui::CalcTextSize(m_GroupData->name.c_str());
        Image::RenderImage(m_GroupDataImages->logo, ImVec2(ImGui::GetWindowWidth() / 2 - text_size.x / 2 - logo_size.x / 2 - gap_text_logo, ImGui::GetWindowHeight() / 8 - logo_size.y / 2), logo_size);
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - text_size.x / 2 + logo_size.x / 2 + gap_text_logo, ImGui::GetWindowHeight() / 8 - text_size.y / 2));
        ImGui::Text("%s", m_GroupData->name.c_str());
        ImGui::PopFont();
        m_AircraftSelectButtonBar.Render();
    }


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
