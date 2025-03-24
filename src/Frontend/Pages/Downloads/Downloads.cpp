
#include "Downloads.hpp"

#include <cmath>
#include <imgui.h>
#include <iostream>

#include "Backend/Downloads/Downloads.hpp"

Downloads::Downloads() {}

void Downloads::Render() {
  ImGui::BeginChild("Downloads", ImGui::GetContentRegionAvail(), false,
                    ImGuiWindowFlags_NoBackground);

  if (ImGui::Button("Download")) {
    Infinity::Downloads::GetInstance().StartDownload(
        "https://link.testfile.org/500MB", "/home/cameron/Downloads/test.file");
  }

  ImGui::Text("Downloads");
  auto& downloader = Infinity::Downloads::GetInstance();
  auto* downloads = downloader.GetAllDownloads();

  std::vector<int> remove_queue;

  for (auto& download: *downloads) {
    ImGui::Text("Download ID: %d", download.first);
    if (download.second.error == -1 || download.second.error == 0) {
      AnimatedProgressBar(download.second.progress, download.second.completed,
                          false, 0.1f);
      ImGui::Text("Progress: %.2f%%",
                  download.second.completed
                      ? 100.0f
                      : download.second.progress * 100.0f);
      auto speed = download.second.speed;
      if (speed > 1024 * 1024) {
        float speedMB = speed / (1024.0f * 1024.0f);
        ImGui::Text("Speed: %.2f MB/s", speedMB);
      } else {
        float speedKB = speed / 1024.0f;
        ImGui::Text("Speed: %.2f KB/s", speedKB);
      }

    } else if (download.second.error == 12) {
      remove_queue.push_back(download.first);
    } else {
      ImGui::Text(
          "Failed with error code: %d A full description of error codes can be "
          "found on the Infinity Docs",
          download.second.error);
    }
    if (ImGui::Button("X")) {
      if (download.second.completed) {
        remove_queue.push_back(download.first);
      } else {
        downloader.StopDownload(download.first);
      }
    }
    ImGui::SameLine();
    if (download.second.paused) {
      if (ImGui::Button("Resume")) {
        Infinity::Downloads::GetInstance().ResumeDownload(download.first);
      }
    } else {
      if (ImGui::Button("Pause")) {
        Infinity::Downloads::GetInstance().PauseDownload(download.first);
      }
    }
  }

  for (int id: remove_queue) {
    downloader.RemoveDownload(id);
  }
  ImGui::EndChild();
}

void Downloads::AnimatedProgressBar(float& progress, bool completed,
                                    bool show_percentage, float smoothness) {
  static float display_progress = 0.0f;
  ImGuiIO& io = ImGui::GetIO();
  float target_progress = completed ? 1.0f : progress;
  display_progress += (target_progress - display_progress) * smoothness;
  display_progress = fmaxf(0.0f, fminf(display_progress, 1.0f));

  ImVec2 bar_size = ImVec2(ImGui::GetContentRegionAvail().x, 20.0f);
  ImVec4 bar_color = completed ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f)
                               : ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
  ImVec4 bg_color = ImVec4(0.2f, 0.2f, 0.2f, 0.8f);

  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  draw_list->AddRectFilled(pos, ImVec2(pos.x + bar_size.x, pos.y + bar_size.y),
                           ImGui::ColorConvertFloat4ToU32(bg_color), 4.0f);

  float fillWidth = bar_size.x * display_progress;
  draw_list->AddRectFilled(pos, ImVec2(pos.x + fillWidth, pos.y + bar_size.y),
                           ImGui::ColorConvertFloat4ToU32(bar_color), 4.0f);

  if (show_percentage) {
    const char* text = completed ? "Complete" : nullptr;
    char buffer[32];
    if (!completed) {
      snprintf(buffer, sizeof(buffer), "%.0f%%", display_progress * 100.0f);
      text = buffer;
    }
    if (text) {
      ImVec2 textSize = ImGui::CalcTextSize(text);
      ImVec2 textPos = ImVec2(pos.x + (bar_size.x - textSize.x) * 0.5f,
                              pos.y + (bar_size.y - textSize.y) * 0.5f);
      draw_list->AddText(textPos, IM_COL32(255, 255, 255, 255), text);
    }
  }

  ImGui::Dummy(bar_size);
}
