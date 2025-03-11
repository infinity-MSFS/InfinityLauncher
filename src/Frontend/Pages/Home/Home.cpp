
#include "Home.hpp"

#include <numeric>

#include "Backend/Application/Application.hpp"
#include "Backend/Router/Router.hpp"
#include "Frontend/Pages/Project/Project.hpp"
#include "Frontend/SVG/SVGDrawing.hpp"
#include "imgui.h"

namespace Infinity {

  unsigned int Home::m_ExpectedProjects = 800000;
  bool Home::m_DoneLoading = false;

  void Home::Render() {
    HandleScrollInput();

    UpdateScrollAnimation();

    ImGui::PushFont(Application::GetFont("DefaultXLarge"));
    auto text_size = ImGui::CalcTextSize("Infinity");
    std::vector<unsigned int> active_index(242 - 1);
    std::iota(active_index.begin(), active_index.end(), 1);
    auto logo = Application::Get().value()->GetIcon();
    Image::RenderImage(logo,
                       ImVec2(ImGui::GetWindowWidth() / 2 - 120.0f, 54.0f),
                       {100.0f, 100.0f});
    ImGui::SetCursorPos(
        ImVec2(ImGui::GetWindowWidth() / 2 - text_size.x / 2 + 50, 80.0f));
    ImGui::Text("Infinity");
    ImGui::PopFont();
    size_t index = 3;  // skip reserved pages
    for (const auto &project: m_HomeProjectButtons) {
      RenderProject(project, index);
      index++;
    }
  }

  void Home::RegisterProject(const std::string &name,
                             std::shared_ptr<Image> image,
                             std::shared_ptr<Image> logo, const int page_id) {
    m_HomeProjectButtons.emplace_back(name, image, logo, page_id);
  }

  void Home::RegisterProject(
      const std::vector<HomeProjectButtonStruct> &projects) {
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

  void Home::HandleScrollInput() {
    float wheel = ImGui::GetIO().MouseWheel;

    if (wheel != 0) {
      // std::cout << "Got update from mouse wheel: " << wheel << std::endl;
      m_TargetScrollOffset += wheel * 100.0f;
      m_IsScrolling = true;
      m_LastScrollTime = ImGui::GetTime();
    }
    float max_scroll;
    float responsive_start;
    int max_scroll_past;  // how many projects can be scrolled past
    if (ImGui::GetWindowWidth() > 1200) {
      responsive_start = ImGui::GetWindowWidth() / 4.3;
      max_scroll_past = 4;
    } else {
      responsive_start = ImGui::GetWindowWidth() / 2.8;
      max_scroll_past = 3;
    }
    if (m_HomeProjectButtons.size() > 4) {
      max_scroll =
          static_cast<float>(m_HomeProjectButtons.size() - max_scroll_past) *
          (responsive_start);
    } else {
      max_scroll =
          static_cast<float>(m_HomeProjectButtons.size()) * (responsive_start);
    }
    m_TargetScrollOffset =
        std::max(0.0f, std::min(m_TargetScrollOffset, max_scroll));
  }
  void Home::UpdateScrollAnimation() {
    if (m_ScrollOffset != m_TargetScrollOffset) {
      float delta_time = ImGui::GetIO().DeltaTime;
      float diff = m_TargetScrollOffset - m_ScrollOffset;
      float step = diff * m_ScrollSpeed * delta_time;

      if (std::abs(diff) < 1.0f) {
        m_ScrollOffset = m_TargetScrollOffset;
      } else {
        m_ScrollOffset += step;
      }
    }

    const float current_time = ImGui::GetTime();
    if (m_IsScrolling && (current_time - m_LastScrollTime > 0.3f)) {
      m_IsScrolling = false;

      float baseX;
      if (ImGui::GetWindowWidth() > 1200) {
        baseX = ImGui::GetWindowWidth() / 4.3;
      } else {
        baseX = ImGui::GetWindowWidth() / 2.8;
      }
      const float currentPosition = m_ScrollOffset / baseX;
      const float fractionalPart =
          currentPosition - std::floor(currentPosition);

      if (fractionalPart < m_SnapThreashold) {
        m_TargetScrollOffset = std::floor(currentPosition) * baseX;
      } else if (fractionalPart > (1.0f - m_SnapThreashold)) {
        m_TargetScrollOffset = std::ceil(currentPosition) * baseX;
      }
    }
  }


  void Home::RenderProject(const HomeProjectButtonStruct &project,
                           const int page_index) {
    float base_x;
    if (ImGui::GetWindowWidth() > 1200) {
      base_x = ImGui::GetWindowWidth() / 4.3;
    } else {
      base_x = ImGui::GetWindowWidth() / 2.8;
    }
    const float x_pos = base_x - base_x + 30.0f +
        static_cast<float>(page_index - 3) * base_x - m_ScrollOffset;
    constexpr float y_pos = 150.0f;
    const ImVec2 position(x_pos, y_pos);
    const ImVec2 size(base_x - 50.0f, ImGui::GetWindowHeight() - 250.0f);


    // TODO: Max logo size
    const ImVec2 logo_size(ImGui::GetWindowWidth() / 4 - 150.0f,
                           ImGui::GetWindowWidth() / 4 - 150.0f);
    const auto logo_position =
        ImVec2(x_pos + size.x / 2 - logo_size.x / 2,
               ImGui::GetWindowHeight() - logo_size.y - 150.0f);

    ImGui::SetCursorPos(position);

    const bool clicked = ImGui::InvisibleButton(project.name.c_str(), size);

    const bool is_hovered = ImGui::IsItemHovered();

    Image::RenderHomeImage(project.image, position, size, is_hovered);
    Image::RenderImage(project.logo, logo_position, logo_size);

    if (clicked) {
      if (const auto router = Utils::Router::getInstance();
          router.has_value()) {
        ProjectPage::ResetState();
        if (auto result = (*router)->setPage(page_index); !result.has_value()) {
          Errors::Error(result.error()).Dispatch();
        }
      }
    }
    ImGui::PushFont(Application::GetFont("DefaultLarge"));
    const auto text_size = ImGui::CalcTextSize(project.name.c_str());
    ImGui::SetCursorPos({x_pos + size.x / 2 - text_size.x / 2,
                         ImGui::GetWindowHeight() - 80.0f});
    ImGui::Text("%s", project.name.c_str());
    ImGui::PopFont();
  }

  Home *Home::GetInstance() {
    static Home instance;
    return &instance;
  }


}  // namespace Infinity
