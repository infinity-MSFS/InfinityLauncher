
#pragma once

#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "GroupStateManager.hpp"
#include "imgui.h"

namespace Infinity {


  // Helper function to render std::optional fields
  template<typename T>
  void RenderOptionalField(const std::string &fieldName, const std::optional<T> &fieldValue) {
    if (fieldValue) {
      ImGui::Text("%s: %s", fieldName.c_str(), fieldValue->c_str());
    } else {
      ImGui::Text("%s: None", fieldName.c_str());
    }
  }

  // Render the `Package` structure
  void RenderPackage(const Package &package) {
    ImGui::Text("Package:");
    ImGui::Indent();
    ImGui::Text("Owner: %s", package.owner.c_str());
    ImGui::Text("Repo Name: %s", package.repoName.c_str());
    ImGui::Text("Version: %s", package.version.c_str());
    ImGui::Text("File Name: %s", package.fileName.c_str());
    ImGui::Unindent();
  }

  // Render the `Project` structure
  void RenderProject(const Project &project) {
    ImGui::Text("Project:");
    ImGui::Indent();
    ImGui::Text("Name: %s", project.name.c_str());
    ImGui::Text("Version: %s", project.version.c_str());
    ImGui::Text("Date: %s", project.date.c_str());
    ImGui::Text("Changelog: %s", project.changelog.c_str());
    ImGui::Text("Overview: %s", project.overview.c_str());
    ImGui::Text("Description: %s", project.description.c_str());
    ImGui::Text("Background: %s", project.background.c_str());

    if (project.pageBackground) {
      ImGui::Text("Page Background: %s", project.pageBackground->c_str());
    } else {
      ImGui::Text("Page Background: None");
    }

    if (project.variants) {
      ImGui::Text("Variants:");
      ImGui::Indent();
      for (const auto &variant: *project.variants) {
        ImGui::BulletText("%s", variant.c_str());
      }
      ImGui::Unindent();
    } else {
      ImGui::Text("Variants: None");
    }

    if (project.package) {
      RenderPackage(*project.package);
    } else {
      ImGui::Text("Package: None");
    }

    ImGui::Unindent();
  }

  // Render the `Palette` structure
  void RenderPalette(const Palette &palette) {
    ImGui::Text("Palette:");
    ImGui::Indent();
    ImGui::Text("Primary: %s", palette.primary.c_str());
    ImGui::Text("Secondary: %s", palette.secondary.c_str());
    ImGui::Unindent();
  }

  // Render the `BetaProject` structure
  void RenderBetaProject(const BetaProject &beta) {
    ImGui::Text("Beta Project:");
    ImGui::Indent();
    ImGui::Text("Background: %s", beta.background.c_str());
    ImGui::Unindent();
  }

  // Render the `GroupData` structure
  void RenderGroupData(const GroupData &group) {
    ImGui::Text("Group Data:");
    ImGui::Indent();
    ImGui::Text("Name: %s", group.name.c_str());
    ImGui::Text("Logo: %s", group.logo.c_str());

    if (group.update) {
      ImGui::Text("Update: %s", *group.update ? "true" : "false");
    } else {
      ImGui::Text("Update: None");
    }

    ImGui::Text("Path: %s", group.path.c_str());
    RenderPalette(group.palette);

    if (group.hide) {
      ImGui::Text("Hide: %s", *group.hide ? "true" : "false");
    } else {
      ImGui::Text("Hide: None");
    }

    RenderBetaProject(group.beta);

    ImGui::Text("Projects:");
    ImGui::Indent();
    for (const auto &project: group.projects) {
      RenderProject(project);
      ImGui::Separator();
    }
    ImGui::Unindent();

    ImGui::Unindent();
  }

  // Render the `GroupDataState` structure
  void RenderGroupDataState(const GroupDataState &state) {
    ImGui::Text("Group Data State:");
    ImGui::Indent();
    for (const auto &[groupName, groupData]: state.groups) {
      ImGui::Text("Group Name: %s", groupName.c_str());
      RenderGroupData(groupData);
      ImGui::Separator();
    }
    ImGui::Unindent();
  }
}  // namespace Infinity
