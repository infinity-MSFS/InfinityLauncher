#include "Markdown.hpp"

#include "Assets/Fonts/IcontsFontAwesome5.h"

namespace ImGui {
  TextRegion::TextRegion()
      : indent_x(0.0f)
      , indent_count(0) {}
  TextRegion::~TextRegion() { ResetIndent(); }

  void TextRegion::RenderTextWrapped(const char *text, const char *text_end, bool indent_to_here) {
    const float scale = GetIO().FontGlobalScale;
    float width_left = GetContentRegionAvail().x;
    const char *end_line = GetFont()->CalcWordWrapPositionA(scale, text, text_end, width_left);
    TextUnformatted(text, end_line);
    if (indent_to_here) {
      if (const float indent = GetContentRegionAvail().x - width_left; indent != 0.0f) {
        Indent();
        indent_x += indent;
        indent_count++;
      }
    }
    width_left = GetContentRegionAvail().x;
    while (end_line < text_end) {
      text = end_line;
      if (*text == ' ') {
        ++text;
      }
      end_line = GetFont()->CalcWordWrapPositionA(scale, text, text_end, width_left);
      if (text == end_line) {
        end_line++;
      }
      TextUnformatted(text, end_line);
    }
  }

  void TextRegion::RenderListTextWrapped(const char *text, const char *text_end) {
    Bullet();
    SameLine();
    RenderTextWrapped(text, text_end, true);
  }

  void TextRegion::ResetIndent() {
    for (int i = 0; i < indent_count; i++) {
      Unindent();
    }
    indent_x = 0.0f;
    indent_count = 0;
  }

  bool TextRegion::RenderLinkText(const char *text, const char *text_end, const Link &link, const char *markdown,
                                  const MarkdownConfig &markdown_config, const char **link_hover_start) {
    MarkdownFormatInfo format_info;
    format_info.config = &markdown_config;
    format_info.type = MarkdownFormatType::LINK;
    markdown_config.format_callback(format_info, true);
    PushTextWrapPos(-1.0f);
    TextUnformatted(text, text_end);
    PopTextWrapPos();

    bool this_item_hovered = IsItemHovered();
    if (this_item_hovered) {
      *link_hover_start = markdown + link.text.start;
    }
    bool hovered = this_item_hovered || (*link_hover_start == (markdown + link.text.start));

    format_info.item_hovered = hovered;
    markdown_config.format_callback(format_info, false);

    if (hovered) {
      if (IsMouseReleased(0) && markdown_config.link_callback) {
        markdown_config.link_callback({markdown + link.text.start, link.text.size(), markdown + link.url.start,
                                       link.url.size(), markdown_config.user_data, false});
      }
      if (markdown_config.tooltip_callback) {
        markdown_config.tooltip_callback({{markdown + link.text.start, link.text.size(), markdown + link.url.start,
                                           link.url.size(), markdown_config.user_data, false},
                                          markdown_config.link_icon});
      }
    }
    return this_item_hovered;
  }

  bool IsCharInWord(char c) {
    return c != ' ' && c != '.' && c != ',' && c != ';' && c != '!' && c != '?' && c != '\"';
  }

  void TextRegion::RenderLinkTextWrapped(const char *text, const char *text_end, const Link &link, const char *markdown,
                                         const MarkdownConfig &markdown_config, const char **link_hover_start,
                                         bool indent_to_here) {
    float scale = GetIO().FontGlobalScale;
    float width_left = GetContentRegionAvail().x;
    const char *end_line = text;
    if (width_left > 0.0f) {
      end_line = GetFont()->CalcWordWrapPositionA(scale, text, text_end, width_left);
    }
    if (end_line > text && end_line < text_end) {
      if (IsCharInWord(*end_line)) {
        float width_next_line = width_left + GetCursorScreenPos().x - GetWindowPos().x;
        const char *end_next_line = GetFont()->CalcWordWrapPositionA(scale, end_line, text_end, width_next_line);
        if (end_next_line == text_end || (end_next_line <= text_end && !IsCharInWord(*end_next_line))) {
          end_line = text;
        }
      }
    }
    bool hovered = RenderLinkText(text, end_line, link, markdown, markdown_config, link_hover_start);
    if (indent_to_here) {
      if (const float indent = GetContentRegionAvail().x - width_left; indent != 0.0f) {
        Indent();
        indent_x += indent;
        indent_count++;
      }
    }
    width_left = GetContentRegionAvail().x;
    while (end_line < text_end) {
      text = end_line;
      if (*text == ' ') {
        ++text;
      }
      end_line = GetFont()->CalcWordWrapPositionA(scale, text, text_end, width_left);
      if (text == end_line) {
        end_line++;
      }
      bool this_line_hovered = RenderLinkText(text, end_line, link, markdown, markdown_config, link_hover_start);
      hovered = hovered || this_line_hovered;
    }
    if (!hovered && *link_hover_start == markdown + link.text.start) {
      *link_hover_start = nullptr;
    }
  }

  void UnderLine(const ImColor col) {
    ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    min.y = max.y;
    GetWindowDrawList()->AddLine(min, max, col, 1.0f);
  }

  void RenderLine(const char *markdown, Line &line, TextRegion &text_region, const MarkdownConfig &markdown_config) {
    int indent_start = 0;
    if (line.is_unordered_list) {
      indent_start = 1;
    }
    for (int i = indent_start; i < line.leading_space_count / 2; i++) {
      Indent();
    }

    MarkdownFormatInfo format_info;
    format_info.config = &markdown_config;
    int text_start = line.last_render_position + 1;
    int text_size = line.line_end - text_start;
    if (line.is_unordered_list) {
      format_info.type = MarkdownFormatType::UNORDERED_LIST;
      markdown_config.format_callback(format_info, true);
      const char *text = markdown + text_start + 1;
      text_region.RenderListTextWrapped(text, text + text_size - 1);
    } else if (line.is_heading) {
      format_info.level = line.heading_count;
      format_info.type = MarkdownFormatType::HEADING;
      markdown_config.format_callback(format_info, true);
      const char *text = markdown + text_start + 1;
      text_region.RenderTextWrapped(text, text + text_size - 1);
    } else if (line.is_emphasis) {
      format_info.level = line.emphasis_count;
      format_info.type = MarkdownFormatType::EMPHASIS;
      markdown_config.format_callback(format_info, true);
      const char *text = markdown + text_start;
      text_region.RenderTextWrapped(text, text + text_size);
    } else {
      format_info.type = MarkdownFormatType::NORMAL_TEXT;
      markdown_config.format_callback(format_info, true);
      const char *text = markdown + text_start;
      text_region.RenderTextWrapped(text, text + text_size);
    }

    markdown_config.format_callback(format_info, false);

    for (int i = indent_start; i < line.leading_space_count / 2; i++) {
      Unindent();
    }
  }


  void DefaultMarkdownFormalCallback(const MarkdownFormatInfo &markdown_format_info, bool start) {
    switch (markdown_format_info.type) {
      case MarkdownFormatType::NORMAL_TEXT:
        break;
      case MarkdownFormatType::EMPHASIS: {
        if (markdown_format_info.level == 1) {
          if (start) {
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
          } else {
            PopStyleColor();
          }
        } else {
          auto [font, seperator] = markdown_format_info.config->heading_formats[MarkdownConfig::NUM_HEADINGS - 1];
          if (start) {
            if (font) {
              PushFont(font);
            }
          } else {
            if (font) {
              PopFont();
            }
          }
        }
        break;
      }
      case MarkdownFormatType::HEADING: {
        MarkdownHeadingFormat heading_format{};
        if (markdown_format_info.level < MarkdownConfig::NUM_HEADINGS) {
          heading_format = markdown_format_info.config->heading_formats[MarkdownConfig::NUM_HEADINGS - 1];
        } else {
          heading_format = markdown_format_info.config->heading_formats[markdown_format_info.level - 1];
        }
        if (start) {
          if (heading_format.font) {
            PushFont(heading_format.font);
          }
          NewLine();
        } else {
          if (heading_format.seperator) {
            Separator();
            NewLine();
          } else {
            NewLine();
          }
          if (heading_format.font) {
            PopFont();
          }
        }
        break;
      }
      case MarkdownFormatType::UNORDERED_LIST:
        break;
      case MarkdownFormatType::LINK: {
        if (start) {
          PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_ButtonHovered]);
        } else {
          PopStyleColor();
          if (markdown_format_info.item_hovered) {
            UnderLine(GetStyle().Colors[ImGuiCol_ButtonHovered]);
          } else {
            UnderLine(GetStyle().Colors[ImGuiCol_Button]);
          }
        }
        break;
      }
    }
  }

  void MarkdownRenderer(const char *markdown, size_t markdown_length, const MarkdownConfig &markdown_config) {
    static const char *linkHoverStart = nullptr;
    ImGuiStyle &style = ImGui::GetStyle();
    Line line;
    Link link;
    Emphasis em;
    TextRegion textRegion;

    char c = 0;
    for (int i = 0; i < static_cast<int>(markdown_length); ++i) {
      c = markdown[i];
      if (c == 0) {
        break;
      }


      if (line.is_leading_space) {
        if (c == ' ') {
          ++line.leading_space_count;
          continue;
        } else {
          line.is_leading_space = false;
          line.last_render_position = i - 1;
          if ((c == '-') && (line.leading_space_count >= 2)) {
            if ((static_cast<int>(markdown_length) > i + 1) && (markdown[i + 1] == ' ')) {
              line.is_unordered_list = true;
              ++i;
              ++line.last_render_position;
            }
            // FIXME: Indents are broken
          } else if (c == '#') {
            line.heading_count++;
            bool bContinueChecking = true;
            int j = i;
            while (++j < static_cast<int>(markdown_length) && bContinueChecking) {
              c = markdown[j];
              switch (c) {
                case '#':
                  line.heading_count++;
                  break;
                case ' ':
                  line.last_render_position = j - 1;
                  i = j;
                  line.is_heading = true;
                  bContinueChecking = false;
                  break;
                default:
                  line.is_heading = false;
                  bContinueChecking = false;
                  break;
              }
            }
            if (line.is_heading) {
              em = Emphasis();
              continue;
            }
          }
        }
      }

      switch (link.state) {
        case Link::NO_LINK:
          if (c == '[' && !line.is_heading) {
            link.state = Link::HAS_SQUARE_BRACKET_OPEN;
            link.text.start = i + 1;
            if (i > 0 && markdown[i - 1] == '!') {
              link.is_image = true;
            }
          }
          break;
        case Link::HAS_SQUARE_BRACKET_OPEN:
          if (c == ']') {
            link.state = Link::HAS_SQUARE_BRACKETS;
            link.text.stop = i;
          }
          break;
        case Link::HAS_SQUARE_BRACKETS:
          if (c == '(') {
            link.state = Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN;
            link.url.start = i + 1;
            link.num_brackets_open = 1;
          }
          break;
        case Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN:
          if (c == '(') {
            ++link.num_brackets_open;
          } else if (c == ')') {
            --link.num_brackets_open;
          }
          if (link.num_brackets_open == 0) {
            em = Emphasis();
            line.line_end = link.text.start - (link.is_image ? 2 : 1);
            RenderLine(markdown, line, textRegion, markdown_config);
            line.leading_space_count = 0;
            link.url.stop = i;
            line.is_unordered_list = false;
            ImGui::SameLine(0.0f, 0.0f);
            if (link.is_image) {
              // dont draw images
            } else {
              textRegion.RenderLinkTextWrapped(markdown + link.text.start,
                                               markdown + link.text.start + link.text.size(), link, markdown,
                                               markdown_config, &linkHoverStart, false);
            }
            ImGui::SameLine(0.0f, 0.0f);
            link = Link();
            line.last_render_position = i;
            break;
          }
      }

      switch (em.state) {
        case Emphasis::NONE:
          if (link.state == Link::NO_LINK && !line.is_heading) {
            int next = i + 1;
            int prev = i - 1;
            if ((c == '*' || c == '_') && (i == line.line_start || markdown[prev] == ' ' || markdown[prev] == '\t') &&
                static_cast<int>(markdown_length) > next && markdown[next] != ' ' && markdown[next] != '\n' &&
                markdown[next] != '\t') {
              em.state = Emphasis::LEFT;
              em.sym = c;
              em.text.start = i;
              line.emphasis_count = 1;
              continue;
            }
          }
          break;
        case Emphasis::LEFT:
          if (em.sym == c) {
            ++line.emphasis_count;
            continue;
          } else {
            em.text.start = i;
            em.state = Emphasis::MIDDLE;
          }
          break;
        case Emphasis::MIDDLE:
          if (em.sym == c) {
            em.state = Emphasis::RIGHT;
            em.text.stop = i;
          } else {
            break;
          }
        case Emphasis::RIGHT:
          if (em.sym == c) {
            if (line.emphasis_count < 3 && (i - em.text.stop + 1 == line.emphasis_count)) {
              int line_end = em.text.start - line.emphasis_count;
              if (line_end > line.line_start) {
                line.line_end = line_end;
                RenderLine(markdown, line, textRegion, markdown_config);
                ImGui::SameLine(0.0f, 0.0f);
                line.is_unordered_list = false;
                line.leading_space_count = 0;
              }
              line.is_emphasis = true;
              line.last_render_position = em.text.start - 1;
              line.line_start = em.text.start;
              line.line_end = em.text.stop;
              RenderLine(markdown, line, textRegion, markdown_config);
              ImGui::SameLine(0.0f, 0.0f);
              line.is_emphasis = false;
              line.last_render_position = i;
              em = Emphasis();
            }
            continue;
          } else {
            em.state = Emphasis::NONE;
            int start = em.text.start - line.emphasis_count;
            if (start < line.line_start) {
              line.line_end = line.line_start;
              line.line_start = start;
              line.last_render_position = start - 1;
              RenderLine(markdown, line, textRegion, markdown_config);
              line.line_start = line.line_end;
              line.last_render_position = line.line_start - 1;
            }
          }
          break;
      }

      if (c == '\n') {
        line.line_end = i;
        if (em.state == Emphasis::MIDDLE && line.emphasis_count >= 3 && (line.line_start + line.emphasis_count) == i) {
          ImGui::Separator();
        } else {
          RenderLine(markdown, line, textRegion, markdown_config);
        }

        line = Line();
        em = Emphasis();

        line.line_start = i + 1;
        line.last_render_position = i;

        textRegion.ResetIndent();

        link = Link();
      }
    }

    if (em.state == Emphasis::LEFT && line.emphasis_count >= 3) {
      ImGui::Separator();
    } else {
      if (markdown_length && line.line_start < static_cast<int>(markdown_length) && markdown[line.line_start] != 0) {
        line.line_end = static_cast<int>(markdown_length);
        if (0 == markdown[line.line_end - 1]) {
          --line.line_end;
        }
        RenderLine(markdown, line, textRegion, markdown_config);
      }
    }
  }


}  // namespace ImGui

namespace Infinity {
  Markdown *Markdown::instance = nullptr;
  Markdown::Markdown()
      : m_Config({LinkCallback,
                  nullptr,
                  ICON_FA_LINK,
                  {{Application::GetFont("h1"), true},
                   {Application::GetFont("h2"), true},
                   {Application::GetFont("h3"), true}},
                  nullptr}) {}

  void Markdown::Render(const std::string &text) const { ImGui::MarkdownRenderer(text.c_str(), text.size(), m_Config); }


  void Markdown::LinkCallback(ImGui::MarkdownLinkCallbackData data) {
    std::string url(data.link, data.link_length);
    if (!data.is_image) {
      OpenUrlInBrowser(url);
    }
  }


}  // namespace Infinity
