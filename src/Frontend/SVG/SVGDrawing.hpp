
#pragma once

#include <cmath>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "imgui.h"

#include "Util/Error/Error.hpp"

static bool printed = false;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dis(0.0f, 1.0f);

std::random_device rd1;
std::mt19937 gen1(rd1());
std::uniform_real_distribution<float> dis1(0.0f, 1.0f);

std::random_device rd2;
std::mt19937 gen2(rd2());
std::uniform_real_distribution<float> dis2(0.0f, 1.0f);

inline void PrintSVGData(const std::vector<std::pair<ImVec2, ImVec2>> &svg_data) {

    if (!printed) {
        for (const auto &segment: svg_data) {
            printf("(%.2f,%.2f)->(%.2f,%.2f)\n", segment.first.x, segment.first.y, segment.second.x, segment.second.y);
        }
    }

    printed = true;
}

class SVGFlatten {
    enum class PathCommand { MoveTo, LineTo, CubicBezier, SmoothCubicBezier, ClosePath };

    struct PathCommandData {
        PathCommand type;
        std::vector<float> coords;
    };

public:
    explicit SVGFlatten(std::string path_raw, const double resolution) :
        m_PathRaw(std::move(path_raw)), m_Resolution(resolution), m_CurrentPoint({0.0, 0.0}), m_LastControlPoint(std::nullopt) {
    }

    std::vector<std::pair<ImVec2, ImVec2>> GetPoints() {
        for (const auto commands = ParsePath(m_PathRaw); const auto &command: commands) {
            auto segments = FlattenCommand(command);
            m_Points.insert(m_Points.end(), segments.begin(), segments.end());
        }
        std::vector<std::pair<ImVec2, ImVec2>> result;

        for (size_t i = 1; i < m_Points.size(); ++i) {
            result.emplace_back(m_Points[i - 1], m_Points[i]);
        }
        return result;
    }

    static void DrawSVG(ImDrawList *draw_list, std::vector<std::pair<ImVec2, ImVec2>> &svg_data, const ImColor color, const float thickness, const bool fill, const ImVec2 offset, const float scale) {
        // if (fill) {
        //     auto *dl = ImGui::GetWindowDrawList();
        //     std::vector<ImVec2> fill_points;
        //     for (auto &[fst, snd]: svg_data) {
        //         fill_points.emplace_back(
        //                 fst.x * scale + offset.x,
        //                 fst.y * scale + offset.y
        //                 );
        //         fill_points.emplace_back(
        //                 snd.x * scale + offset.x,
        //                 snd.y * scale + offset.y
        //                 );
        //     }
        //
        //     std::ranges::sort(fill_points,
        //                       [](const ImVec2 &a, const ImVec2 &b) {
        //                           return std::tie(a.x, a.y) < std::tie(b.x, b.y);
        //                       });
        //     fill_points.erase(
        //             std::ranges::unique(fill_points,
        //                                 [](const ImVec2 &a, const ImVec2 &b) {
        //                                     return a.x == b.x && a.y == b.y;
        //                                 }).begin(),
        //             fill_points.end()
        //             );
        //     std::cout << "fill_points: " << fill_points.data() << std::endl;
        //     dl->AddConvexPolyFilled(fill_points.data(), static_cast<int>(fill_points.size()), color);
        // } else {
        for (auto &i: svg_data) {
            float random = dis(gen);
            float random1 = dis1(gen2);
            float random2 = dis2(gen2);
            std::cout << random << std::endl;
            draw_list->AddLine({i.first.x * scale + offset.x, i.first.y * scale + offset.y}, {i.second.x * scale + offset.x, i.second.y * scale + offset.y}, ImColor(random1, random, random2, 1.0f),
                               thickness);
        }

        //  }
    }

    void write_points_to_file(const std::vector<std::pair<ImVec2, ImVec2>> &points, const std::string &filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file");
        }

        for (size_t i = 1; i < points.size(); ++i) {
            file << "DrawLogoLine({" << points[i].first.x << ", " << points[i].first.y << "}, {" << points[i].second.x << ", " << points[i].second.y << "}, scale, offset);\n";
        }
    }

private:
    static ImVec2 cubic_bezier(const float t, const ImVec2 &p0, const ImVec2 &p1, const ImVec2 &p2, const ImVec2 &p3) {
        const float u = 1.0f - t;
        const float tt = t * t;
        const float uu = u * u;
        const float uuu = uu * u;
        const float ttt = tt * t;

        return {uuu * p0.x + 3.0f * uu * t * p1.y + 3.0f * u * tt * p2.x + ttt * p3.y, uuu * p0.x + 3.0f * uu * t * p1.y + 3.0f * u * tt * p2.x + ttt * p3.y};
    }

    std::vector<ImVec2> FlattenCommand(const PathCommandData &command) {
        std::vector<ImVec2> result;
        switch (command.type) {

            case PathCommand::LineTo:
            case PathCommand::MoveTo: {
                m_CurrentPoint = {command.coords[0], command.coords[1]};
                result.push_back(m_CurrentPoint);
                break;
            }
            case PathCommand::CubicBezier: {
                const auto start = m_CurrentPoint;
                const ImVec2 control1 = {command.coords[0], command.coords[1]};
                ImVec2 control2 = {command.coords[2], command.coords[3]};
                const ImVec2 end = {command.coords[4], command.coords[5]};

                for (double t = 0.0; t <= 1.0; t += m_Resolution) {
                    auto point = cubic_bezier(t, start, control1, control2, end);
                    result.push_back(point);
                }
                result.push_back(end);

                m_LastControlPoint = control2;
                m_CurrentPoint = end;
                break;
            }
            case PathCommand::SmoothCubicBezier: {
                const auto start = m_CurrentPoint;
                const ImVec2 control1 = m_LastControlPoint.has_value() ? ImVec2(start.x + (start.x - m_LastControlPoint->x), start.y + (start.y - m_LastControlPoint->y)) : start;
                ImVec2 control2 = {command.coords[0], command.coords[1]};
                const ImVec2 end = {command.coords[2], command.coords[3]};

                for (double t = 0.0; t <= 1.0; t += m_Resolution) {
                    auto point = cubic_bezier(t, start, control1, control2, end);
                    result.push_back(point);
                }
                result.push_back(end);

                m_LastControlPoint = control2;
                m_CurrentPoint = end;
                break;
            }
            case PathCommand::ClosePath:
                break;
        }
        return result;
    }

    static float ParseCoordinates(const std::string &path_data, size_t pos) {
        while (pos < path_data.size() && (std::isspace(path_data[pos]) || path_data[pos] == ',')) {
            pos++;
        }
        const size_t start = pos;

        if (pos < path_data.length() && (path_data[pos] == '-' || path_data[pos] == '+'))
            pos++;

        while (pos < path_data.length() && (std::isdigit(path_data[pos]) || path_data[pos] == '.')) {
            pos++;
        }
        const std::string coord_string = path_data.substr(start, pos - start);
        return coord_string.empty() ? 0.0f : std::stof(coord_string);
    }

    static std::vector<PathCommandData> ParsePath(const std::string &path_data) {
        std::vector<PathCommandData> result;
        size_t pos = 0;

        while (pos < path_data.length()) {
            const char cmd = path_data[pos];
            pos++;

            switch (cmd) {
                case 'm':
                case 'l':
                case 'c':
                case 's':
                case 'z': {
                    auto error = Infinity::Errors::Error{Infinity::Errors::ErrorType::NonFatal, "Relative Paths commands not supported"};
                    error.Dispatch();
                }
                case 'M': {
                    float x = ParseCoordinates(path_data, pos);
                    float y = ParseCoordinates(path_data, pos);
                    result.push_back({PathCommand::MoveTo, {x, y}});
                    break;
                }
                case 'L': {
                    float x = ParseCoordinates(path_data, pos);
                    float y = ParseCoordinates(path_data, pos);
                    result.push_back({PathCommand::LineTo, {x, y}});
                    break;
                }
                case 'C': {
                    float x1 = ParseCoordinates(path_data, pos);
                    float y1 = ParseCoordinates(path_data, pos);
                    float x2 = ParseCoordinates(path_data, pos);
                    float y2 = ParseCoordinates(path_data, pos);
                    float x = ParseCoordinates(path_data, pos);
                    float y = ParseCoordinates(path_data, pos);
                    result.push_back({PathCommand::CubicBezier, {x1, y1, x2, y2, x, y}});
                    break;
                }
                case 'S': {
                    float x2 = ParseCoordinates(path_data, pos);
                    float y2 = ParseCoordinates(path_data, pos);
                    float x = ParseCoordinates(path_data, pos);
                    float y = ParseCoordinates(path_data, pos);
                    result.push_back({PathCommand::SmoothCubicBezier, {x2, y2, x, y}});
                    break;
                }
                case 'Z': {
                    result.push_back({PathCommand::ClosePath, {}});
                    break;
                }
                default:
                    break;
            }
        }
        return result;
    }

private:
    std::string m_PathRaw;
    std::vector<ImVec2> m_Points;
    double m_Resolution;
    ImVec2 m_CurrentPoint;
    std::optional<ImVec2> m_LastControlPoint;
};


inline void DrawLogoLine(const ImVec2 pos1, const ImVec2 pos2, const float scale = 1.0f, const ImVec2 offset = {0, 0}, const float thickness = 1.0f, int index = 0,
                         const std::vector<unsigned int> &active_index = {}) {
    bool is_active = false;
    float opacity_multiplier = 1.0f;

    for (size_t i = 0; i < active_index.size(); i++) {
        if (active_index[i] == index) {
            is_active = true;

            if (i < 3) {
                opacity_multiplier = 0.4f + (0.3f * i);
            } else if (i >= active_index.size() - 3) {
                size_t position_from_end = active_index.size() - 1 - i;
                opacity_multiplier = 0.4f + (0.3f * position_from_end);
            }
            break;
        }
    }

    const auto dl = ImGui::GetWindowDrawList();

    ImVec2 transformed_pos1 = {
            (pos1.x - 520.0f) * scale + offset.x,
            (pos1.y - 215.0f) * scale + offset.y
    };

    ImVec2 transformed_pos2 = {
            (pos2.x - 520.0f) * scale + offset.x,
            (pos2.y - 215.0f) * scale + offset.y
    };

    if (is_active) {

        const int num_layers = 6;
        const float glow_intensity = 0.5f;
        const float max_extra_thickness = thickness * 2.0f;

        for (int i = 0; i < num_layers; i++) {
            float t = static_cast<float>(i) / (num_layers - 1);
            float current_thickness = thickness + (max_extra_thickness * t);
            float alpha = glow_intensity * (1.0f - t) * opacity_multiplier;

            ImColor glow_color = ImColor(1.0f, 1.0f, 1.0f, alpha);
            dl->AddLine(transformed_pos1, transformed_pos2, glow_color, current_thickness);
        }

        // Draw main line
        dl->AddLine(transformed_pos1, transformed_pos2,
                    ImColor(1.0f, 1.0f, 1.0f, opacity_multiplier), thickness);
    } else {
        // Draw inactive line
        dl->AddLine(transformed_pos1, transformed_pos2, ImColor(0.39f, 0.39f, 0.39f, 0.39f), thickness);
    }
}


// Generated by Rust function -> TODO: Repo link
inline void DrawInfinityLogoAnimated(const float scale = 1.0f, const ImVec2 offset = {0, 0}, const std::vector<unsigned int> &active_index = {}) {
    // ~800px wide
    /// ~430px tall
    // 295
    constexpr float thickness = 2.0f;
    DrawLogoLine({280.80, 728.90}, {280.80, 728.90}, scale, offset, thickness, 0, active_index);
    DrawLogoLine({280.80, 728.90}, {263.57, 728.27}, scale, offset, thickness, 1, active_index);
    DrawLogoLine({263.57, 728.27}, {246.56, 726.29}, scale, offset, thickness, 2, active_index);
    DrawLogoLine({246.56, 726.29}, {229.83, 722.98}, scale, offset, thickness, 3, active_index);
    DrawLogoLine({229.83, 722.98}, {213.46, 718.38}, scale, offset, thickness, 4, active_index);
    DrawLogoLine({213.46, 718.38}, {197.54, 712.51}, scale, offset, thickness, 5, active_index);
    DrawLogoLine({197.54, 712.51}, {182.12, 705.41}, scale, offset, thickness, 6, active_index);
    DrawLogoLine({182.12, 705.41}, {167.30, 697.11}, scale, offset, thickness, 7, active_index);
    DrawLogoLine({167.30, 697.11}, {153.14, 687.64}, scale, offset, thickness, 8, active_index);
    DrawLogoLine({153.14, 687.64}, {139.71, 677.02}, scale, offset, thickness, 9, active_index);
    DrawLogoLine({139.71, 677.02}, {127.10, 665.30}, scale, offset, thickness, 10, active_index);
    DrawLogoLine({127.10, 665.30}, {127.10, 665.30}, scale, offset, thickness, 11, active_index);
    DrawLogoLine({127.10, 665.30}, {127.10, 665.30}, scale, offset, thickness, 12, active_index);
    DrawLogoLine({127.10, 665.30}, {104.34, 638.41}, scale, offset, thickness, 13, active_index);
    DrawLogoLine({104.34, 638.41}, {86.64, 608.97}, scale, offset, thickness, 14, active_index);
    DrawLogoLine({86.64, 608.97}, {73.99, 577.62}, scale, offset, thickness, 15, active_index);
    DrawLogoLine({73.99, 577.62}, {66.40, 545.01}, scale, offset, thickness, 16, active_index);
    DrawLogoLine({66.40, 545.01}, {63.86, 511.76}, scale, offset, thickness, 17, active_index);
    DrawLogoLine({63.86, 511.76}, {66.38, 478.52}, scale, offset, thickness, 18, active_index);
    DrawLogoLine({66.38, 478.52}, {73.96, 445.91}, scale, offset, thickness, 19, active_index);
    DrawLogoLine({73.96, 445.91}, {86.58, 414.58}, scale, offset, thickness, 20, active_index);
    DrawLogoLine({86.58, 414.58}, {104.27, 385.17}, scale, offset, thickness, 21, active_index);
    DrawLogoLine({104.27, 385.17}, {127.00, 358.30}, scale, offset, thickness, 22, active_index);
    DrawLogoLine({127.00, 358.30}, {127.00, 358.30}, scale, offset, thickness, 23, active_index);
    DrawLogoLine({127.00, 358.30}, {127.00, 358.30}, scale, offset, thickness, 24, active_index);
    DrawLogoLine({127.00, 358.30}, {139.64, 346.55}, scale, offset, thickness, 25, active_index);
    DrawLogoLine({139.64, 346.55}, {153.09, 335.91}, scale, offset, thickness, 26, active_index);
    DrawLogoLine({153.09, 335.91}, {167.27, 326.42}, scale, offset, thickness, 27, active_index);
    DrawLogoLine({167.27, 326.42}, {182.11, 318.11}, scale, offset, thickness, 28, active_index);
    DrawLogoLine({182.11, 318.11}, {197.54, 311.00}, scale, offset, thickness, 29, active_index);
    DrawLogoLine({197.54, 311.00}, {213.48, 305.13}, scale, offset, thickness, 30, active_index);
    DrawLogoLine({213.48, 305.13}, {229.86, 300.52}, scale, offset, thickness, 31, active_index);
    DrawLogoLine({229.86, 300.52}, {246.61, 297.21}, scale, offset, thickness, 32, active_index);
    DrawLogoLine({246.61, 297.21}, {263.64, 295.23}, scale, offset, thickness, 33, active_index);
    DrawLogoLine({263.64, 295.23}, {280.90, 294.60}, scale, offset, thickness, 34, active_index);
    DrawLogoLine({280.90, 294.60}, {280.90, 294.60}, scale, offset, thickness, 35, active_index);
    DrawLogoLine({280.90, 294.60}, {280.90, 294.60}, scale, offset, thickness, 36, active_index);
    DrawLogoLine({280.90, 294.60}, {298.21, 295.27}, scale, offset, thickness, 37, active_index);
    DrawLogoLine({298.21, 295.27}, {315.26, 297.28}, scale, offset, thickness, 38, active_index);
    DrawLogoLine({315.26, 297.28}, {331.97, 300.59}, scale, offset, thickness, 39, active_index);
    DrawLogoLine({331.97, 300.59}, {348.29, 305.18}, scale, offset, thickness, 40, active_index);
    DrawLogoLine({348.29, 305.18}, {364.16, 311.02}, scale, offset, thickness, 41, active_index);
    DrawLogoLine({364.16, 311.02}, {379.52, 318.10}, scale, offset, thickness, 42, active_index);
    DrawLogoLine({379.52, 318.10}, {394.30, 326.38}, scale, offset, thickness, 43, active_index);
    DrawLogoLine({394.30, 326.38}, {408.45, 335.84}, scale, offset, thickness, 44, active_index);
    DrawLogoLine({408.45, 335.84}, {421.90, 346.46}, scale, offset, thickness, 45, active_index);
    DrawLogoLine({421.90, 346.46}, {434.60, 358.20}, scale, offset, thickness, 46, active_index);
    DrawLogoLine({434.60, 358.20}, {550.00, 473.70}, scale, offset, thickness, 47, active_index);


    DrawLogoLine({549.90, 473.80}, {665.40, 589.30}, scale, offset, thickness, 48, active_index);
    DrawLogoLine({665.40, 589.30}, {665.40, 589.30}, scale, offset, thickness, 49, active_index);
    DrawLogoLine({665.40, 589.30}, {678.72, 600.53}, scale, offset, thickness, 50, active_index);
    DrawLogoLine({678.72, 600.53}, {693.51, 609.26}, scale, offset, thickness, 51, active_index);
    DrawLogoLine({693.51, 609.26}, {709.42, 615.49}, scale, offset, thickness, 52, active_index);
    DrawLogoLine({709.42, 615.49}, {726.07, 619.22}, scale, offset, thickness, 53, active_index);
    DrawLogoLine({726.07, 619.22}, {743.09, 620.46}, scale, offset, thickness, 54, active_index);
    DrawLogoLine({743.09, 620.46}, {760.10, 619.21}, scale, offset, thickness, 55, active_index);
    DrawLogoLine({760.10, 619.21}, {776.74, 615.46}, scale, offset, thickness, 56, active_index);
    DrawLogoLine({776.74, 615.46}, {792.64, 609.23}, scale, offset, thickness, 57, active_index);
    DrawLogoLine({792.64, 609.23}, {807.41, 600.51}, scale, offset, thickness, 58, active_index);
    DrawLogoLine({807.41, 600.51}, {820.70, 589.30}, scale, offset, thickness, 59, active_index);
    DrawLogoLine({820.70, 589.30}, {820.70, 589.30}, scale, offset, thickness, 60, active_index);
    DrawLogoLine({820.70, 589.30}, {820.70, 589.30}, scale, offset, thickness, 61, active_index);
    DrawLogoLine({820.70, 589.30}, {832.17, 575.69}, scale, offset, thickness, 62, active_index);
    DrawLogoLine({832.17, 575.69}, {841.10, 560.82}, scale, offset, thickness, 63, active_index);
    DrawLogoLine({841.10, 560.82}, {847.47, 545.01}, scale, offset, thickness, 64, active_index);
    DrawLogoLine({847.47, 545.01}, {851.29, 528.56}, scale, offset, thickness, 65, active_index);
    DrawLogoLine({851.29, 528.56}, {852.56, 511.80}, scale, offset, thickness, 66, active_index);
    DrawLogoLine({852.56, 511.80}, {851.28, 495.04}, scale, offset, thickness, 67, active_index);
    DrawLogoLine({851.28, 495.04}, {847.44, 478.59}, scale, offset, thickness, 68, active_index);
    DrawLogoLine({847.44, 478.59}, {841.05, 462.78}, scale, offset, thickness, 69, active_index);
    DrawLogoLine({841.05, 462.78}, {832.10, 447.91}, scale, offset, thickness, 70, active_index);
    DrawLogoLine({832.10, 447.91}, {820.60, 434.30}, scale, offset, thickness, 71, active_index);
    DrawLogoLine({820.60, 434.30}, {820.60, 434.30}, scale, offset, thickness, 72, active_index);
    DrawLogoLine({820.60, 434.30}, {820.60, 434.30}, scale, offset, thickness, 73, active_index);
    DrawLogoLine({820.60, 434.30}, {807.36, 423.15}, scale, offset, thickness, 74, active_index);
    DrawLogoLine({807.36, 423.15}, {792.62, 414.47}, scale, offset, thickness, 75, active_index);
    DrawLogoLine({792.62, 414.47}, {776.75, 408.26}, scale, offset, thickness, 76, active_index);
    DrawLogoLine({776.75, 408.26}, {760.13, 404.54}, scale, offset, thickness, 77, active_index);
    DrawLogoLine({760.13, 404.54}, {743.12, 403.30}, scale, offset, thickness, 78, active_index);
    DrawLogoLine({743.12, 403.30}, {726.11, 404.54}, scale, offset, thickness, 79, active_index);
    DrawLogoLine({726.11, 404.54}, {709.47, 408.27}, scale, offset, thickness, 80, active_index);
    DrawLogoLine({709.47, 408.27}, {693.57, 414.49}, scale, offset, thickness, 81, active_index);
    DrawLogoLine({693.57, 414.49}, {678.79, 423.20}, scale, offset, thickness, 82, active_index);
    DrawLogoLine({678.79, 423.20}, {665.50, 434.40}, scale, offset, thickness, 83, active_index);
    DrawLogoLine({665.50, 434.40}, {665.50, 434.40}, scale, offset, thickness, 84, active_index);
    DrawLogoLine({665.50, 434.40}, {651.50, 448.40}, scale, offset, thickness, 85, active_index);
    DrawLogoLine({651.50, 448.40}, {651.50, 448.40}, scale, offset, thickness, 86, active_index);
    DrawLogoLine({651.50, 448.40}, {644.83, 454.07}, scale, offset, thickness, 87, active_index);
    DrawLogoLine({644.83, 454.07}, {637.54, 458.48}, scale, offset, thickness, 88, active_index);
    DrawLogoLine({637.54, 458.48}, {629.77, 461.63}, scale, offset, thickness, 89, active_index);
    DrawLogoLine({629.77, 461.63}, {621.69, 463.52}, scale, offset, thickness, 90, active_index);
    DrawLogoLine({621.69, 463.52}, {613.45, 464.15}, scale, offset, thickness, 91, active_index);
    DrawLogoLine({613.45, 464.15}, {605.21, 463.52}, scale, offset, thickness, 92, active_index);
    DrawLogoLine({605.21, 463.52}, {597.13, 461.63}, scale, offset, thickness, 93, active_index);
    DrawLogoLine({597.13, 461.63}, {589.36, 458.48}, scale, offset, thickness, 94, active_index);
    DrawLogoLine({589.36, 458.48}, {582.07, 454.07}, scale, offset, thickness, 95, active_index);
    DrawLogoLine({582.07, 454.07}, {575.40, 448.40}, scale, offset, thickness, 96, active_index);
    DrawLogoLine({575.40, 448.40}, {575.40, 448.40}, scale, offset, thickness, 97, active_index);
    DrawLogoLine({575.40, 448.40}, {575.40, 448.40}, scale, offset, thickness, 98, active_index);
    DrawLogoLine({575.40, 448.40}, {569.73, 441.73}, scale, offset, thickness, 99, active_index);
    DrawLogoLine({569.73, 441.73}, {565.32, 434.44}, scale, offset, thickness, 100, active_index);
    DrawLogoLine({565.32, 434.44}, {562.17, 426.67}, scale, offset, thickness, 101, active_index);
    DrawLogoLine({562.17, 426.67}, {560.28, 418.59}, scale, offset, thickness, 102, active_index);
    DrawLogoLine({560.28, 418.59}, {559.65, 410.35}, scale, offset, thickness, 103, active_index);
    DrawLogoLine({559.65, 410.35}, {560.28, 402.11}, scale, offset, thickness, 104, active_index);
    DrawLogoLine({560.28, 402.11}, {562.17, 394.03}, scale, offset, thickness, 105, active_index);
    DrawLogoLine({562.17, 394.03}, {565.32, 386.26}, scale, offset, thickness, 106, active_index);
    DrawLogoLine({565.32, 386.26}, {569.73, 378.97}, scale, offset, thickness, 107, active_index);
    DrawLogoLine({569.73, 378.97}, {575.40, 372.30}, scale, offset, thickness, 108, active_index);
    DrawLogoLine({575.40, 372.30}, {575.40, 372.30}, scale, offset, thickness, 109, active_index);
    DrawLogoLine({575.40, 372.30}, {589.40, 358.30}, scale, offset, thickness, 110, active_index);
    DrawLogoLine({589.40, 358.30}, {589.40, 358.30}, scale, offset, thickness, 111, active_index);
    DrawLogoLine({589.40, 358.30}, {602.04, 346.58}, scale, offset, thickness, 112, active_index);
    DrawLogoLine({602.04, 346.58}, {615.47, 335.96}, scale, offset, thickness, 113, active_index);
    DrawLogoLine({615.47, 335.96}, {629.64, 326.49}, scale, offset, thickness, 114, active_index);
    DrawLogoLine({629.64, 326.49}, {644.47, 318.19}, scale, offset, thickness, 115, active_index);
    DrawLogoLine({644.47, 318.19}, {659.88, 311.09}, scale, offset, thickness, 116, active_index);
    DrawLogoLine({659.88, 311.09}, {675.79, 305.22}, scale, offset, thickness, 117, active_index);
    DrawLogoLine({675.79, 305.22}, {692.15, 300.62}, scale, offset, thickness, 118, active_index);
    DrawLogoLine({692.15, 300.62}, {708.87, 297.31}, scale, offset, thickness, 119, active_index);
    DrawLogoLine({708.87, 297.31}, {725.87, 295.33}, scale, offset, thickness, 120, active_index);
    DrawLogoLine({725.87, 295.33}, {743.10, 294.70}, scale, offset, thickness, 121, active_index);
    DrawLogoLine({743.10, 294.70}, {743.10, 294.70}, scale, offset, thickness, 122, active_index);
    DrawLogoLine({743.10, 294.70}, {743.10, 294.70}, scale, offset, thickness, 123, active_index);
    DrawLogoLine({743.10, 294.70}, {760.41, 295.37}, scale, offset, thickness, 124, active_index);
    DrawLogoLine({760.41, 295.37}, {777.46, 297.38}, scale, offset, thickness, 125, active_index);
    DrawLogoLine({777.46, 297.38}, {794.17, 300.69}, scale, offset, thickness, 126, active_index);
    DrawLogoLine({794.17, 300.69}, {810.49, 305.28}, scale, offset, thickness, 127, active_index);
    DrawLogoLine({810.49, 305.28}, {826.36, 311.12}, scale, offset, thickness, 128, active_index);
    DrawLogoLine({826.36, 311.12}, {841.72, 318.20}, scale, offset, thickness, 129, active_index);
    DrawLogoLine({841.72, 318.20}, {856.50, 326.48}, scale, offset, thickness, 130, active_index);
    DrawLogoLine({856.50, 326.48}, {870.65, 335.94}, scale, offset, thickness, 131, active_index);
    DrawLogoLine({870.65, 335.94}, {884.10, 346.56}, scale, offset, thickness, 132, active_index);
    DrawLogoLine({884.10, 346.56}, {896.80, 358.30}, scale, offset, thickness, 133, active_index);
    DrawLogoLine({896.80, 358.30}, {896.80, 358.30}, scale, offset, thickness, 134, active_index);
    DrawLogoLine({896.80, 358.30}, {896.80, 358.30}, scale, offset, thickness, 135, active_index);
    DrawLogoLine({896.80, 358.30}, {919.56, 385.19}, scale, offset, thickness, 136, active_index);
    DrawLogoLine({919.56, 385.19}, {937.26, 414.63}, scale, offset, thickness, 137, active_index);
    DrawLogoLine({937.26, 414.63}, {949.91, 445.98}, scale, offset, thickness, 138, active_index);
    DrawLogoLine({949.91, 445.98}, {957.50, 478.59}, scale, offset, thickness, 139, active_index);
    DrawLogoLine({957.50, 478.59}, {960.04, 511.84}, scale, offset, thickness, 140, active_index);
    DrawLogoLine({960.04, 511.84}, {957.52, 545.08}, scale, offset, thickness, 141, active_index);
    DrawLogoLine({957.52, 545.08}, {949.94, 577.69}, scale, offset, thickness, 142, active_index);
    DrawLogoLine({949.94, 577.69}, {937.32, 609.02}, scale, offset, thickness, 143, active_index);
    DrawLogoLine({937.32, 609.02}, {919.63, 638.43}, scale, offset, thickness, 144, active_index);
    DrawLogoLine({919.63, 638.43}, {896.90, 665.30}, scale, offset, thickness, 145, active_index);
    DrawLogoLine({896.90, 665.30}, {896.90, 665.30}, scale, offset, thickness, 146, active_index);
    DrawLogoLine({896.90, 665.30}, {896.90, 665.30}, scale, offset, thickness, 147, active_index);
    DrawLogoLine({896.90, 665.30}, {884.26, 677.00}, scale, offset, thickness, 148, active_index);
    DrawLogoLine({884.26, 677.00}, {870.82, 687.60}, scale, offset, thickness, 149, active_index);
    DrawLogoLine({870.82, 687.60}, {856.65, 697.07}, scale, offset, thickness, 150, active_index);
    DrawLogoLine({856.65, 697.07}, {841.83, 705.37}, scale, offset, thickness, 151, active_index);
    DrawLogoLine({841.83, 705.37}, {826.41, 712.48}, scale, offset, thickness, 152, active_index);
    DrawLogoLine({826.41, 712.48}, {810.49, 718.35}, scale, offset, thickness, 153, active_index);
    DrawLogoLine({810.49, 718.35}, {794.12, 722.96}, scale, offset, thickness, 154, active_index);
    DrawLogoLine({794.12, 722.96}, {777.38, 726.28}, scale, offset, thickness, 155, active_index);
    DrawLogoLine({777.38, 726.28}, {760.35, 728.27}, scale, offset, thickness, 156, active_index);
    DrawLogoLine({760.35, 728.27}, {743.10, 728.90}, scale, offset, thickness, 157, active_index);
    DrawLogoLine({743.10, 728.90}, {743.10, 728.90}, scale, offset, thickness, 158, active_index);


    DrawLogoLine({743.10, 728.90}, {725.87, 728.27}, scale, offset, thickness, 159, active_index);
    DrawLogoLine({725.87, 728.27}, {708.87, 726.29}, scale, offset, thickness, 160, active_index);
    DrawLogoLine({708.87, 726.29}, {692.15, 722.98}, scale, offset, thickness, 161, active_index);
    DrawLogoLine({692.15, 722.98}, {675.79, 718.38}, scale, offset, thickness, 162, active_index);
    DrawLogoLine({675.79, 718.38}, {659.88, 712.51}, scale, offset, thickness, 163, active_index);
    DrawLogoLine({659.88, 712.51}, {644.47, 705.41}, scale, offset, thickness, 164, active_index);
    DrawLogoLine({644.47, 705.41}, {629.64, 697.11}, scale, offset, thickness, 165, active_index);
    DrawLogoLine({629.64, 697.11}, {615.47, 687.64}, scale, offset, thickness, 166, active_index);
    DrawLogoLine({615.47, 687.64}, {602.04, 677.02}, scale, offset, thickness, 167, active_index);
    DrawLogoLine({602.04, 677.02}, {589.40, 665.30}, scale, offset, thickness, 168, active_index);
    DrawLogoLine({589.40, 665.30}, {473.90, 549.80}, scale, offset, thickness, 169, active_index);
    DrawLogoLine({474.00, 549.70}, {358.50, 434.30}, scale, offset, thickness, 170, active_index);
    DrawLogoLine({358.50, 434.30}, {345.18, 423.10}, scale, offset, thickness, 171, active_index);
    DrawLogoLine({345.18, 423.10}, {330.39, 414.38}, scale, offset, thickness, 172, active_index);
    DrawLogoLine({330.39, 414.38}, {314.48, 408.16}, scale, offset, thickness, 173, active_index);
    DrawLogoLine({314.48, 408.16}, {297.83, 404.42}, scale, offset, thickness, 174, active_index);
    DrawLogoLine({297.83, 404.42}, {280.81, 403.18}, scale, offset, thickness, 175, active_index);
    DrawLogoLine({280.81, 403.18}, {263.80, 404.42}, scale, offset, thickness, 176, active_index);
    DrawLogoLine({263.80, 404.42}, {247.16, 408.15}, scale, offset, thickness, 177, active_index);
    DrawLogoLine({247.16, 408.15}, {231.26, 414.38}, scale, offset, thickness, 178, active_index);
    DrawLogoLine({231.26, 414.38}, {216.49, 423.10}, scale, offset, thickness, 179, active_index);
    DrawLogoLine({216.49, 423.10}, {203.20, 434.30}, scale, offset, thickness, 180, active_index);
    DrawLogoLine({203.20, 434.30}, {203.20, 434.30}, scale, offset, thickness, 181, active_index);
    DrawLogoLine({203.20, 434.30}, {203.20, 434.30}, scale, offset, thickness, 182, active_index);
    DrawLogoLine({203.20, 434.30}, {191.73, 447.91}, scale, offset, thickness, 183, active_index);
    DrawLogoLine({191.73, 447.91}, {182.80, 462.78}, scale, offset, thickness, 184, active_index);
    DrawLogoLine({182.80, 462.78}, {176.43, 478.59}, scale, offset, thickness, 185, active_index);
    DrawLogoLine({176.43, 478.59}, {172.61, 495.04}, scale, offset, thickness, 186, active_index);
    DrawLogoLine({172.61, 495.04}, {171.34, 511.80}, scale, offset, thickness, 187, active_index);
    DrawLogoLine({171.34, 511.80}, {172.62, 528.56}, scale, offset, thickness, 188, active_index);
    DrawLogoLine({172.62, 528.56}, {176.46, 545.01}, scale, offset, thickness, 189, active_index);
    DrawLogoLine({176.46, 545.01}, {182.85, 560.82}, scale, offset, thickness, 190, active_index);
    DrawLogoLine({182.85, 560.82}, {191.80, 575.69}, scale, offset, thickness, 191, active_index);
    DrawLogoLine({191.80, 575.69}, {203.30, 589.30}, scale, offset, thickness, 192, active_index);
    DrawLogoLine({203.30, 589.30}, {216.54, 600.43}, scale, offset, thickness, 193, active_index);
    DrawLogoLine({216.54, 600.43}, {231.28, 609.09}, scale, offset, thickness, 194, active_index);
    DrawLogoLine({231.28, 609.09}, {247.15, 615.29}, scale, offset, thickness, 195, active_index);
    DrawLogoLine({247.15, 615.29}, {263.77, 619.02}, scale, offset, thickness, 196, active_index);
    DrawLogoLine({263.77, 619.02}, {280.77, 620.26}, scale, offset, thickness, 197, active_index);
    DrawLogoLine({280.77, 620.26}, {297.79, 619.03}, scale, offset, thickness, 198, active_index);
    DrawLogoLine({297.79, 619.03}, {314.43, 615.31}, scale, offset, thickness, 199, active_index);
    DrawLogoLine({314.43, 615.31}, {330.33, 609.10}, scale, offset, thickness, 200, active_index);
    DrawLogoLine({330.33, 609.10}, {345.11, 600.40}, scale, offset, thickness, 201, active_index);
    DrawLogoLine({345.11, 600.40}, {358.40, 589.20}, scale, offset, thickness, 202, active_index);
    DrawLogoLine({358.40, 589.20}, {358.40, 589.20}, scale, offset, thickness, 203, active_index);
    DrawLogoLine({358.40, 589.20}, {372.40, 575.20}, scale, offset, thickness, 204, active_index);
    DrawLogoLine({372.40, 575.20}, {372.40, 575.20}, scale, offset, thickness, 205, active_index);
    DrawLogoLine({372.40, 575.20}, {379.07, 569.53}, scale, offset, thickness, 206, active_index);
    DrawLogoLine({379.07, 569.53}, {386.36, 565.12}, scale, offset, thickness, 207, active_index);
    DrawLogoLine({386.36, 565.12}, {394.13, 561.97}, scale, offset, thickness, 208, active_index);
    DrawLogoLine({394.13, 561.97}, {402.21, 560.08}, scale, offset, thickness, 209, active_index);
    DrawLogoLine({402.21, 560.08}, {410.45, 559.45}, scale, offset, thickness, 210, active_index);
    DrawLogoLine({410.45, 559.45}, {418.69, 560.08}, scale, offset, thickness, 211, active_index);
    DrawLogoLine({418.69, 560.08}, {426.77, 561.97}, scale, offset, thickness, 212, active_index);
    DrawLogoLine({426.77, 561.97}, {434.54, 565.12}, scale, offset, thickness, 213, active_index);
    DrawLogoLine({434.54, 565.12}, {441.83, 569.53}, scale, offset, thickness, 214, active_index);
    DrawLogoLine({441.83, 569.53}, {448.50, 575.20}, scale, offset, thickness, 215, active_index);
    DrawLogoLine({448.50, 575.20}, {448.50, 575.20}, scale, offset, thickness, 216, active_index);
    DrawLogoLine({448.50, 575.20}, {448.50, 575.20}, scale, offset, thickness, 217, active_index);
    DrawLogoLine({448.50, 575.20}, {454.17, 581.87}, scale, offset, thickness, 218, active_index);
    DrawLogoLine({454.17, 581.87}, {458.58, 589.16}, scale, offset, thickness, 219, active_index);
    DrawLogoLine({458.58, 589.16}, {461.73, 596.93}, scale, offset, thickness, 220, active_index);
    DrawLogoLine({461.73, 596.93}, {463.62, 605.01}, scale, offset, thickness, 221, active_index);
    DrawLogoLine({463.62, 605.01}, {464.25, 613.25}, scale, offset, thickness, 222, active_index);
    DrawLogoLine({464.25, 613.25}, {463.62, 621.49}, scale, offset, thickness, 223, active_index);
    DrawLogoLine({463.62, 621.49}, {461.73, 629.57}, scale, offset, thickness, 224, active_index);
    DrawLogoLine({461.73, 629.57}, {458.58, 637.34}, scale, offset, thickness, 225, active_index);
    DrawLogoLine({458.58, 637.34}, {454.17, 644.63}, scale, offset, thickness, 226, active_index);
    DrawLogoLine({454.17, 644.63}, {448.50, 651.30}, scale, offset, thickness, 227, active_index);
    DrawLogoLine({448.50, 651.30}, {448.50, 651.30}, scale, offset, thickness, 228, active_index);
    DrawLogoLine({448.50, 651.30}, {434.50, 665.30}, scale, offset, thickness, 229, active_index);
    DrawLogoLine({434.50, 665.30}, {434.50, 665.30}, scale, offset, thickness, 230, active_index);
    DrawLogoLine({434.50, 665.30}, {421.86, 677.02}, scale, offset, thickness, 231, active_index);
    DrawLogoLine({421.86, 677.02}, {408.42, 687.64}, scale, offset, thickness, 232, active_index);
    DrawLogoLine({408.42, 687.64}, {394.24, 697.11}, scale, offset, thickness, 233, active_index);
    DrawLogoLine({394.24, 697.11}, {379.40, 705.41}, scale, offset, thickness, 234, active_index);
    DrawLogoLine({379.40, 705.41}, {363.99, 712.51}, scale, offset, thickness, 235, active_index);
    DrawLogoLine({363.99, 712.51}, {348.06, 718.38}, scale, offset, thickness, 236, active_index);
    DrawLogoLine({348.06, 718.38}, {331.71, 722.98}, scale, offset, thickness, 237, active_index);
    DrawLogoLine({331.71, 722.98}, {315.00, 726.29}, scale, offset, thickness, 238, active_index);
    DrawLogoLine({315.00, 726.29}, {298.00, 728.27}, scale, offset, thickness, 239, active_index);
    DrawLogoLine({298.00, 728.27}, {280.80, 728.90}, scale, offset, thickness, 240, active_index);
    DrawLogoLine({280.80, 728.90}, {280.80, 728.90}, scale, offset, thickness, 241, active_index);
}



