#pragma once


#include "GL/glew.h"
//
#include <cstdint>
#include <imgui.h>
#include <string>
#ifdef WIN32
#include <Windows.h>
#endif
#include <cstdlib>

#include "Backend/Application/Application.hpp"

namespace ImGui {
  struct Link;
  struct MarkdownConfig;

  struct MarkdownLinkCallbackData {
    const char *text;
    int text_length;
    const char *link;
    int link_length;
    void *user_data;
    bool is_image;
  };

  struct MarkdownTooltipCallbackData {
    MarkdownLinkCallbackData link_data;
    const char *link_icon;
  };


  enum class MarkdownFormatType {
    NORMAL_TEXT,
    HEADING,
    UNORDERED_LIST,
    LINK,
    EMPHASIS,
  };

  struct MarkdownFormatInfo {
    MarkdownFormatType type = MarkdownFormatType::NORMAL_TEXT;
    int32_t level = 0;
    bool item_hovered = false;
    const MarkdownConfig *config = nullptr;
  };

  typedef void MarkdownLinkCallback(MarkdownLinkCallbackData data);
  typedef void MarkdownTooltipCallback(MarkdownTooltipCallbackData data);

  inline void DefaultMarkdownTooltipCallback(
      const MarkdownTooltipCallbackData &data_) {
    if (data_.link_data.is_image) {
      SetTooltip("%.*s", data_.link_data.link_length, data_.link_data.link);
    } else {
      SetTooltip("%s Open in browser\n%.*s", data_.link_icon,
                 data_.link_data.link_length, data_.link_data.link);
    }
  }

  typedef void MarkdownFormalCallback(
      const MarkdownFormatInfo &markdown_format_info, bool start);

  inline void DefaultMarkdownFormalCallback(
      const MarkdownFormatInfo &markdown_format_info, bool start);

  struct MarkdownHeadingFormat {
    ImFont *font;
    bool seperator;
  };

  struct MarkdownConfig {
    static constexpr int NUM_HEADINGS = 3;

    MarkdownLinkCallback *link_callback = nullptr;
    MarkdownTooltipCallback *tooltip_callback = nullptr;
    const char *link_icon = "";
    MarkdownHeadingFormat heading_formats[NUM_HEADINGS] = {
        {nullptr, true}, {nullptr, true}, {nullptr, true}};
    void *user_data = nullptr;
    MarkdownFormalCallback *format_callback = DefaultMarkdownFormalCallback;
  };

  void MarkdownRenderer(const char *markdown, size_t markdown_length,
                        const MarkdownConfig &markdown_config);

  struct TextRegion;
  struct Line;
  inline void UnderLine(ImColor col);
  inline void RenderLine(const char *markdown, Line &line,
                         TextRegion &text_region,
                         const MarkdownConfig &markdown_config);

  struct TextRegion {
private:
    float indent_x;

public:
    TextRegion();
    ~TextRegion();
    void RenderTextWrapped(const char *text, const char *text_end,
                           bool indent_to_here = false);
    void RenderListTextWrapped(const char *text, const char *text_end);
    static bool RenderLinkText(const char *text, const char *text_end,
                               const Link &link, const char *markdown,
                               const MarkdownConfig &markdown_config,
                               const char **link_hover_start);
    void RenderLinkTextWrapped(const char *text, const char *text_end,
                               const Link &link, const char *markdown,
                               const MarkdownConfig &markdown_config,
                               const char **link_hover_start,
                               bool indent_to_here = false);
    void ResetIndent();
  };

  struct Line {
    bool is_heading = false;
    bool is_emphasis = false;
    bool is_unordered_list = false;
    bool is_leading_space = true;
    int leading_space_count = 0;
    int heading_count = 0;
    int emphasis_count = 0;
    int line_start = 0;
    int line_end = 0;
    int last_render_position = 0;
  };

  struct TextBlock {
    int start = 0;
    int stop = 0;
    [[nodiscard]] int size() const { return stop - start; }
  };
  struct Link {
    enum LinkState {
      NO_LINK,
      HAS_SQUARE_BRACKET_OPEN,
      HAS_SQUARE_BRACKETS,
      HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN,
    };
    LinkState state = NO_LINK;
    TextBlock text;
    TextBlock url;
    bool is_image = false;
    int num_brackets_open = 0;
  };

  struct Emphasis {
    enum EmphasisState { NONE, LEFT, MIDDLE, RIGHT };

    EmphasisState state = NONE;
    TextBlock text;
    char sym{};
  };


}  // namespace ImGui


inline void OpenUrlInBrowser(std::string &url) {
#ifdef WIN32
  ShellExecute(nullptr, "open", url.c_str(), nullptr, nullptr, SW_HIDE);

#elif __APPLE__
  std::string command = "open " + url;
  std::system(command.c_str());
#elif __linux__
  std::string command = "xdg-open " + url;
  std::system(command.c_str());
#endif
}

namespace Infinity {
  class Markdown {
public:
    void Render(const std::string &text) const;
    static Markdown *GetInstance() {
      if (instance == nullptr) {
        instance = new Markdown();
      }
      return instance;
    }

private:
    Markdown();

    static void LinkCallback(ImGui::MarkdownLinkCallbackData data);

    ImGui::MarkdownConfig m_Config;

    static Markdown *instance;
  };
}  // namespace Infinity
