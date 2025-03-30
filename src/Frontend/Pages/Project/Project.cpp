
#include "Project.hpp"

#include <imgui.h>
#include <utility>

#include "Backend/Image/Image.hpp"
#include "Backend/Image/SvgImage.hpp"
#include "Frontend/Markdown/Markdown.hpp"
#include "Util/State/State.hpp"

namespace Infinity {

  std::shared_ptr<uint8_t> ProjectPage::m_SelectedAircraft = std::make_shared<uint8_t>(0);
  std::shared_ptr<uint8_t> ProjectPage::m_SelectedPage = std::make_shared<uint8_t>(0);

  ProjectPage::ProjectPage(const std::shared_ptr<GroupData> &group_data,
                           const std::shared_ptr<GroupDataImages> &state_images)
      : m_GroupData(group_data)
      , m_StateImages(state_images)
      , m_ContentRegion(m_GroupData, m_SelectedPage, m_SelectedAircraft)
      , m_TopRegion(m_GroupData, m_StateImages, m_SelectedAircraft) {}


  void ProjectPage::Render() {
    auto &state = State::GetInstance();
    if (const auto main_state = state.GetPageState<MainState>("main"); main_state.has_value()) {
      if (!m_StateImages->projectImages.empty()) {
#ifdef WIN32
        constexpr float top_padding = 40.0f;
#else
        constexpr float top_padding = 0.0f;
#endif

        if (!m_StateImages->projectImages.empty() && m_StateImages->projectImages[0].pageBackgroundImage.has_value()) {
          Image::RenderImage(m_StateImages->projectImages[0].pageBackgroundImage.value(), {0.0f, top_padding},
                             {ImGui::GetWindowWidth(), ImGui::GetWindowHeight() / 3.0f}, 0.3f);
          m_TopRegion.Render();
          m_ContentRegion.Render();
        } else if (m_StateImages->projectImages[0].backgroundImage) {
          Image::RenderImage(m_StateImages->projectImages[0].backgroundImage, {0.0f, top_padding},
                             {ImGui::GetWindowWidth(), ImGui::GetWindowHeight() / 3.0f}, 0.3f);
          m_TopRegion.Render();
          m_ContentRegion.Render();
        }
      }
    }
  }


  AircraftSelectButton::AircraftSelectButton(std::string name, int32_t id,
                                             const std::shared_ptr<uint8_t> &selected_aircraft)
      : m_Name(std::move(name))
      , m_Id(id)
      , m_Active(false)
      , m_SelectedAircraft(selected_aircraft) {}

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
      draw_list->AddRectFilled(cursor_pos, ImVec2(cursor_pos.x + size.x, cursor_pos.y + size.y),
                               ImGui::GetColorU32(bg_color), 2.0f);
      draw_list->AddTriangleFilled(
          active_arrow_location, ImVec2(active_arrow_location.x + active_arrow_size.x, active_arrow_location.y),
          ImVec2(active_arrow_location.x + active_arrow_size.x / 2, active_arrow_location.y + active_arrow_size.y),
          ImGui::GetColorU32(bg_color));
    }
    if (!active)
      draw_list->AddRect(cursor_pos, ImVec2(cursor_pos.x + size.x, cursor_pos.y + size.y),
                         ImGui::GetColorU32(border_color), 2.0f, 0, 2.0f);


    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos = ImVec2(cursor_pos.x + (size.x - textSize.x) * 0.5f, cursor_pos.y + (size.y - textSize.y) * 0.5f);
    draw_list->AddText(textPos, ImGui::GetColorU32(text_color), label);

    return pressed;
  }

  AircraftSelectButtonBar::AircraftSelectButtonBar(const std::vector<AircraftSelectButton> &aircraft_select_buttons,
                                                   const std::shared_ptr<uint8_t> &selected_aircraft)
      : m_AircraftSelectButtons(std::move(aircraft_select_buttons))
      , m_SelectedAircraft(selected_aircraft) {}

  void AircraftSelectButtonBar::Render() {
    const int button_count = m_AircraftSelectButtons.size();
    constexpr float padding = 40.0f;  // padding on left and right side of screen
    constexpr float gap = 10.0f;  // gap between buttons
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

  TopRegion::TopRegion(const std::shared_ptr<GroupData> &group_data,
                       const std::shared_ptr<GroupDataImages> &group_data_images,
                       const std::shared_ptr<uint8_t> &selected_aircraft)
      : m_GroupData(group_data)
      , m_GroupDataImages(group_data_images)
      , m_AircraftSelectButtonBar({}, selected_aircraft)
      , m_SelectedAircraft(selected_aircraft) {
    for (int32_t i = 0; i < group_data->projects.size(); i++) {
      m_AircraftSelectButtons.emplace_back(AircraftSelectButton(group_data->projects[i].name, i, selected_aircraft));
    }
    m_AircraftSelectButtonBar = AircraftSelectButtonBar(m_AircraftSelectButtons, selected_aircraft);
  }

  void TopRegion::Render() {
    ImGui::PushFont(Application::GetFont("BoldXLarge"));
    const float gap_text_logo = 10.0f;
    const ImVec2 logo_size(150.0f, 150.0f);
    const auto text_size = ImGui::CalcTextSize(m_GroupData->name.c_str());
    Image::RenderImage(m_GroupDataImages->logo,
                       ImVec2(ImGui::GetWindowWidth() / 2 - text_size.x / 2 - logo_size.x / 2 - gap_text_logo,
                              ImGui::GetWindowHeight() / 8 - logo_size.y / 2),
                       logo_size);
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - text_size.x / 2 + logo_size.x / 2 + gap_text_logo,
                               ImGui::GetWindowHeight() / 8 - text_size.y / 2));
    ImGui::Text("%s", m_GroupData->name.c_str());
    ImGui::PopFont();
    m_AircraftSelectButtonBar.Render();
  }


  ContentRegionButton::ContentRegionButton(const std::string &name, int8_t id,
                                           const std::shared_ptr<uint8_t> &selected_page)
      : m_ButtonSpec(name, id)
      , m_SelectedPage(selected_page) {}

  ContentRegionButton::ContentRegionButton(const Button &button, const std::shared_ptr<uint8_t> &selected_page)
      : m_ButtonSpec(button)
      , m_SelectedPage(selected_page) {}


  void ContentRegionButton::Render(ImVec2 size, ImVec2 pos) {
    if (ButtonRender(m_ButtonSpec.name.c_str(), size, *m_SelectedPage == m_ButtonSpec.page)) {
      *m_SelectedPage = m_ButtonSpec.page;
    }
  }

  bool ContentRegionButton::ButtonRender(const char *label, ImVec2 size, bool active) {
    auto cursor_pos = ImGui::GetCursorScreenPos();

    bool pressed = ImGui::InvisibleButton(label, size);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec4 border_color;
    ImVec4 text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ;

    if (active) {
      border_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
      border_color = ImVec4(1.0f, 1.0f, 1.0f, 0.4f);
    }


    if (active) {
      draw_list->AddLine({cursor_pos.x, cursor_pos.y + size.y}, {cursor_pos.x + size.x, cursor_pos.y + size.y},
                         ImGui::GetColorU32(border_color), 2.0f);
    }
    if (!active)
      draw_list->AddLine({cursor_pos.x, cursor_pos.y + size.y}, {cursor_pos.x + size.x, cursor_pos.y + size.y},
                         ImGui::GetColorU32(border_color), 2.0f);


    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos = ImVec2(cursor_pos.x + (size.x - textSize.x) * 0.5f, cursor_pos.y + (size.y - textSize.y) * 0.5f);
    draw_list->AddText(textPos, ImGui::GetColorU32(text_color), label);

    return pressed;
  }


  ContentRegionButtonBar::ContentRegionButtonBar(std::vector<ContentRegionButton> buttons,
                                                 const std::shared_ptr<uint8_t> &selected_page)
      : m_Buttons(std::move(buttons))
      , m_SelectedPage(selected_page) {}

  void ContentRegionButtonBar::Render() {
    int index = 0;
    const float padding = 40.0f;
    const float gap = 0.0f;
    const float button_width =
        (ImGui::GetWindowWidth() - padding * 2 - gap * (m_Buttons.size() - 1)) / m_Buttons.size();
    const ImVec2 button_size(button_width, 50.0f);
    for (auto &button: m_Buttons) {
      ImGui::SetCursorPos({padding + index * (button_width + gap), ImGui::GetWindowHeight() / 3.0f + 130.0f});
      button.Render(button_size, ImVec2(padding + index * (button_width + gap), ImGui::GetWindowHeight() / 3.0f));
      index++;
    }
  }

  ContentRegion::ContentRegion(const std::shared_ptr<GroupData> &group_data,
                               const std::shared_ptr<uint8_t> &selected_page,
                               const std::shared_ptr<uint8_t> &selected_aircraft)
      : m_GroupData(group_data)
      , m_ButtonBar({}, selected_page)
      , m_SelectedPage(selected_page)
      , m_SelectedAircraft(selected_aircraft) {
    auto overview_button = ContentRegionButton("Overview", 0, m_SelectedPage);
    auto description_button = ContentRegionButton("Description", 1, m_SelectedPage);
    auto changelog_button = ContentRegionButton("Changelog", 2, m_SelectedPage);
    m_ButtonBar = ContentRegionButtonBar({overview_button, description_button, changelog_button}, m_SelectedPage);
  }

  static std::shared_ptr<SVGImage> m_SVGImage = nullptr;

  void ContentRegion::RenderInstalledWidget() {
    ImGui::GetWindowDrawList()->AddRectFilled({40.0f, ImGui::GetWindowHeight() / 3.0f + 10.0f},
                                              {200.0f, ImGui::GetWindowHeight() / 3.0f + 40.0f},
                                              ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.5f)), 10.0f);

    ImGui::GetWindowDrawList()->AddText({45.0f, ImGui::GetWindowHeight() / 3.0f + 10.0f + 5.0f},
                                        ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.6f)), "Not Installed");
    if (ImGui::Button("TestSVG")) {
      auto svg = SVGImage::LoadFromURL("https://www.svgrepo.com/show/765/library.svg");
      m_SVGImage = svg;
    }

    if (m_SVGImage) {
      SVGImage::RenderSVG(m_SVGImage, {40.0f, ImGui::GetWindowHeight() / 3.0f + 10.0f}, 1.0f);
    }
  }

  bool ContentRegion::RenderDownloadButton(const char *label, ImVec2 size, ImVec2 pos) {
    ImGui::SetCursorPos(pos);


    bool pressed = ImGui::InvisibleButton(label, size);

    bool hovered = ImGui::IsItemHovered();

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec4 bg_color, border_color, text_color;

    if (hovered) {
      bg_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
      border_color = bg_color;
      text_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    } else {
      bg_color = ImVec4(0, 0, 0, 0);
      border_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
      text_color = border_color;
    }

    if (hovered) {
      draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::GetColorU32(bg_color), 2.0f);
    }
    if (!hovered)
      draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::GetColorU32(border_color), 2.0f, 0, 2.0f);


    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos = ImVec2(pos.x + (size.x - textSize.x) * 0.5f, pos.y + (size.y - textSize.y) * 0.5f);
    draw_list->AddText(textPos, ImGui::GetColorU32(text_color), label);

    return pressed;
  }

  bool ContentRegion::RenderBugReportButton(ImVec2 size, ImVec2 pos) {
    ImGui::SetCursorPos(pos);

    auto label = "Report Bug";

    bool pressed = ImGui::InvisibleButton(label, size);

    bool hovered = ImGui::IsItemHovered();

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec4 bg_color, border_color, text_color;

    if (hovered) {
      bg_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
      border_color = bg_color;
      text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
      bg_color = ImVec4(1.0f, 0.0f, 0.0f, 0.2f);
      border_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
      text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (hovered) {
      draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::GetColorU32(bg_color), 2.0f);
    }
    if (!hovered)
      draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::GetColorU32(bg_color), 2.0f);
    draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::GetColorU32(border_color), 2.0f, 0, 2.0f);


    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos = ImVec2(pos.x + (size.x - textSize.x) * 0.5f, pos.y + (size.y - textSize.y) * 0.5f);
    draw_list->AddText(textPos, ImGui::GetColorU32(text_color), label);

    return pressed;
  }


  void ContentRegion::Render() {
    RenderInstalledWidget();

    auto download_label = "Download Version " + m_GroupData->projects[*m_SelectedAircraft].version;
    if (RenderDownloadButton(
            download_label.c_str(), {200.0f, 60.0f},
            {ImGui::GetWindowWidth() - 100.0f - 60.0f - 10.0f - 200.0f, ImGui::GetWindowHeight() / 3.0f + 50.0f})) {
      std::cout << "Download button pressed for: " << m_GroupData->projects[*m_SelectedAircraft].name << std::endl;
    }
    if (RenderBugReportButton({120.0f, 60.0f},
                              {ImGui::GetWindowWidth() - 60.0f - 100.0f, ImGui::GetWindowHeight() / 3.0f + 50.0f})) {
      std::cout << "Bug report button pressed for: " << m_GroupData->projects[*m_SelectedAircraft].name << std::endl;
    }

    ImGui::SetCursorPos({40.0f, ImGui::GetWindowHeight() / 3.0f + 50.0f});
    ImGui::PushFont(Application::GetFont("BoldXLarge"));
    const auto header_size = ImGui::CalcTextSize(m_GroupData->projects[*m_SelectedAircraft].name.c_str());
    ImGui::Text("%s", m_GroupData->projects[*m_SelectedAircraft].name.c_str());
    ImGui::PopFont();
    ImGui::SetCursorPosX(40.0f);
    ImGui::PushFont(Application::GetFont("Bold"));
    const auto subheader_size = ImGui::CalcTextSize(m_GroupData->name.c_str());
    ImGui::Text("%s", m_GroupData->name.c_str());
    ImGui::PopFont();
    m_ButtonBar.Render();


    switch (*m_SelectedPage) {
      case 0: {
        ImGui::Text("Overview: %s", m_GroupData->projects[*m_SelectedAircraft].overview.c_str());
        break;
      }
      case 1: {
        Markdown::GetInstance()->Render(m_GroupData->projects[*m_SelectedAircraft].description);

        break;
      }
      case 2: {
        ImGui::Text("Changelog: %s", m_GroupData->projects[*m_SelectedAircraft].changelog.c_str());
        break;
      }
      default:
        break;
    }
  }

}  // namespace Infinity
