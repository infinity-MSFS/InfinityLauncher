
#include "Settings.hpp"

#include <GL/glew.h>
//

#include <imgui.h>
#include <string>

#include "Backend/Application/Application.hpp"
#include "Backend/HWID/Hwid.hpp"
#include "Backend/Updater/Updater.hpp"


namespace Infinity {
  void Settings::Render() {
    ImGui::Text("Settings");
    // if (ImGui::Button("Update")) {
    //   auto pid_dir = Infinity::Updater::GetConfigDir() + "/pid.infinitypid";
    //   Infinity::Updater::WritePidToFile(pid_dir);
    //
    //   auto exe_folder = Infinity::Updater::GetCurrentExecutablePath();
    //   std::string updater_path = exe_folder + "/Updater.exe";
    //   std::string current_exe = exe_folder + "/InfinityLauncher.exe";
    //   std::string new_exe = exe_folder + "/UPDATE_Infinity";
    //   Infinity::Updater::LaunchUpdater(updater_path, new_exe, current_exe, pid_dir);
    // }
    // ImGui::Text("Updated InfinityLauncher :)");

    auto fps = Application::Get().value()->GetFPS();
    float fps_limit = fps;
    ImGui::SliderFloat("FPS Limit", &fps_limit, 60.0f, 240.0f, "%.1f FPS", ImGuiSliderFlags_AlwaysClamp);
    Application::Get().value()->SetFPS(fps_limit);

    auto reduce_fps_on_unfocus = Application::Get().value()->IsReduceFPSOnIdle();
    bool reduce_fps_on_unfocus_bool = reduce_fps_on_unfocus;
    ImGui::Checkbox("Reduce FPS on Unfocus", &reduce_fps_on_unfocus_bool);
    Application::Get().value()->SetReduceFPSOnIdle(reduce_fps_on_unfocus_bool);

    ImGui::Separator();

    if (ImGui::Button("Copy HWID")) {
      HWID hwid;
      auto hwid_string = hwid.GetHWID();
      ImGui::SetClipboardText(hwid_string.c_str());
    }
  }


}  // namespace Infinity
