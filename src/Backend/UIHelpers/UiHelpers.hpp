
#pragma once


#include <memory>

#include "Backend/Image/Image.hpp"
#include "Backend/Image/SvgImage.hpp"
#include "imgui.h"
#include "imgui_internal.h"

namespace Infinity {
  void ShiftCursorX(float distance);

  void ShiftCursorY(float distance);

  void ShiftCursor(float x, float y);

  ImRect GetItemRect();

  ImRect RectExpanded(const ImRect &rect, float x, float y);

  ImRect RectOffset(const ImRect &rect, float x, float y);

  ImRect RectOffset(const ImRect &rect, ImVec2 xy);

  void DrawButtonImage(const std::shared_ptr<Image> &imageNormal, const std::shared_ptr<Image> &imageHovered,
                       const std::shared_ptr<Image> &imagePressed, ImU32 tintNormal, ImU32 tintHovered,
                       ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax);

  void DrawButtonImage(const std::shared_ptr<Image> &imageNormal, const std::shared_ptr<Image> &imageHovered,
                       const std::shared_ptr<Image> &imagePressed, ImU32 tintNormal, ImU32 tintHovered,
                       ImU32 tintPressed, ImRect rectangle);

  void DrawButtonImage(const std::shared_ptr<Image> &image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
                       ImVec2 rectMin, ImVec2 rectMax);

  void DrawButtonImage(const std::shared_ptr<Image> &image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
                       ImRect rectangle);

  void DrawButtonImage(const std::shared_ptr<Image> &imageNormal, const std::shared_ptr<Image> &imageHovered,
                       const std::shared_ptr<Image> &imagePressed, ImU32 tintNormal, ImU32 tintHovered,
                       ImU32 tintPressed);

  void DrawButtonImage(const std::shared_ptr<Image> &image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed);

  void DrawSVGButtonImage(const std::shared_ptr<SVGImage> &svgNormal, const std::shared_ptr<SVGImage> &svgHovered,
                          const std::shared_ptr<SVGImage> &svgPressed, const ImU32 tintNormal, const ImU32 tintHovered,
                          const ImU32 tintPressed, const ImVec2 rectMin, const ImVec2 rectMax);

  void DrawSVGButtonImage(const std::shared_ptr<SVGImage> &svgNormal, const std::shared_ptr<SVGImage> &svgHovered,
                          const std::shared_ptr<SVGImage> &svgPressed, const ImU32 tintNormal, const ImU32 tintHovered,
                          const ImU32 tintPressed, const ImRect rectangle);

  void DrawSVGButtonImage(const std::shared_ptr<SVGImage> &svg, const ImU32 tintNormal, const ImU32 tintHovered,
                          const ImU32 tintPressed, const ImVec2 rectMin, const ImVec2 rectMax);

  void DrawSVGButtonImage(const std::shared_ptr<SVGImage> &svg, const ImU32 tintNormal, const ImU32 tintHovered,
                          const ImU32 tintPressed, const ImRect rectangle);

  void DrawSVGButtonImage(const std::shared_ptr<SVGImage> &svgNormal, const std::shared_ptr<SVGImage> &svgHovered,
                          const std::shared_ptr<SVGImage> &svgPressed, const ImU32 tintNormal, const ImU32 tintHovered,
                          const ImU32 tintPressed);

  void DrawSVGButtonImage(const std::shared_ptr<SVGImage> &svg, const ImU32 tintNormal, const ImU32 tintHovered,
                          const ImU32 tintPressed);

  void DrawSVGButtonImage(const std::shared_ptr<SVGImage> &svgNormal, const std::shared_ptr<SVGImage> &svgHovered,
                          const std::shared_ptr<SVGImage> &svgPressed, const std::string &colorNormal,
                          const std::string &colorHovered, const std::string &colorPressed, const ImVec2 rectMin,
                          const ImVec2 rectMax);

  void DrawSVGButtonImage(const std::shared_ptr<SVGImage> &svg, const std::string &colorNormal,
                          const std::string &colorHovered, const std::string &colorPressed, const ImVec2 rectMin,
                          const ImVec2 rectMax);

  void DrawSVGButtonImage(const std::shared_ptr<SVGImage> &svg, const std::string &colorNormal,
                          const std::string &colorHovered, const std::string &colorPressed);

  void RenderWindowOuterBorders(ImGuiWindow *window);

  bool UpdateWindowManualResize(ImGuiWindow *window, ImVec2 &newSize, ImVec2 &newPosition);

  // Menubar with custom rectangle
  bool BeginMenubar(const ImRect &barRectangle);

  void EndMenubar();

  bool ButtonCentered(const char *label, const ImVec2 &size = ImVec2(0, 0));

  // Utilities
  class ScopedStyle {
public:
    ScopedStyle(const ScopedStyle &) = delete;

    ScopedStyle operator=(const ScopedStyle &) = delete;

    template<typename T>
    ScopedStyle(ImGuiStyleVar styleVar, T value) {
      ImGui::PushStyleVar(styleVar, value);
    }

    ~ScopedStyle() { ImGui::PopStyleVar(); }
  };

  class ScopedColor {
public:
    ScopedColor(const ScopedColor &) = delete;

    ScopedColor operator=(const ScopedColor &) = delete;

    template<typename T>
    ScopedColor(ImGuiCol ColorId, T Color) {
      ImGui::PushStyleColor(ColorId, Color);
    }

    ~ScopedColor() { ImGui::PopStyleColor(); }
  };

  class ScopedFont {
public:
    ScopedFont(const ScopedFont &) = delete;

    ScopedFont operator=(const ScopedFont &) = delete;

    explicit ScopedFont(ImFont *font) { ImGui::PushFont(font); }
    ~ScopedFont() { ImGui::PopFont(); }
  };

  class ScopedID {
public:
    ScopedID(const ScopedID &) = delete;

    ScopedID operator=(const ScopedID &) = delete;

    template<typename T>
    explicit ScopedID(T id) {
      ImGui::PushID(id);
    }

    ~ScopedID() { ImGui::PopID(); }
  };

  class ScopedColorStack {
public:
    ScopedColorStack(const ScopedColorStack &) = delete;

    ScopedColorStack operator=(const ScopedColorStack &) = delete;

    template<typename ColorType, typename... OtherColors>
    ScopedColorStack(ImGuiCol firstColorID, ColorType firstColor, OtherColors &&...otherColorPairs)
        : m_Count((sizeof...(otherColorPairs) / 2) + 1) {
      static_assert((sizeof...(otherColorPairs) & 1u) == 0,
                    "ScopedColorStack constructor expects a list of pairs of "
                    "Color IDs and Colors as its arguments");

      PushColor(firstColorID, firstColor, std::forward<OtherColors>(otherColorPairs)...);
    }

    ~ScopedColorStack() { ImGui::PopStyleColor(m_Count); }

private:
    int m_Count;

    template<typename ColorType, typename... OtherColors>
    void PushColor(ImGuiCol ColorID, ColorType Color, OtherColors &&...otherColorPairs) {
      if constexpr (sizeof...(otherColorPairs) == 0) {
        ImGui::PushStyleColor(ColorID, Color);
      } else {
        ImGui::PushStyleColor(ColorID, Color);
        PushColor(std::forward<OtherColors>(otherColorPairs)...);
      }
    }
  };

  class ScopedStyleStack {
public:
    ScopedStyleStack(const ScopedStyleStack &) = delete;

    ScopedStyleStack operator=(const ScopedStyleStack &) = delete;

    template<typename ValueType, typename... OtherStylePairs>
    ScopedStyleStack(ImGuiStyleVar firstStyleVar, ValueType firstValue, OtherStylePairs &&...otherStylePairs)
        : m_Count((sizeof...(otherStylePairs) / 2) + 1) {
      static_assert((sizeof...(otherStylePairs) & 1u) == 0,
                    "ScopedStyleStack constructor expects a list of pairs of "
                    "Color IDs and Colors as its arguments");

      PushStyle(firstStyleVar, firstValue, std::forward<OtherStylePairs>(otherStylePairs)...);
    }

    ~ScopedStyleStack() { ImGui::PopStyleVar(m_Count); }

private:
    int m_Count;

    template<typename ValueType, typename... OtherStylePairs>
    void PushStyle(ImGuiStyleVar styleVar, ValueType value, OtherStylePairs &&...otherStylePairs) {
      if constexpr (sizeof...(otherStylePairs) == 0) {
        ImGui::PushStyleVar(styleVar, value);
      } else {
        ImGui::PushStyleVar(styleVar, value);
        PushStyle(std::forward<OtherStylePairs>(otherStylePairs)...);
      }
    }
  };

  class ScopedItemFlags {
public:
    ScopedItemFlags(const ScopedItemFlags &) = delete;

    ScopedItemFlags operator=(const ScopedItemFlags &) = delete;

    explicit ScopedItemFlags(const ImGuiItemFlags flags, const bool enable = true) {
      ImGui::PushItemFlag(flags, enable);
    }

    ~ScopedItemFlags() { ImGui::PopItemFlag(); }
  };

  void DrawBorder(ImRect rect, float thickness = 1.0f, float rounding = 0.0f, float offsetX = 0.0f,
                  float offsetY = 0.0f);
}  // namespace Infinity
