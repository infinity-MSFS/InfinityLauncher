#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Image.hpp"

struct NSVGimage;

namespace Infinity {
  class SVGImage : public Image {
public:
    SVGImage();
    ~SVGImage();

    SVGImage(const SVGImage&) = delete;
    SVGImage& operator=(const SVGImage&) = delete;
    SVGImage(SVGImage&& other) noexcept;
    SVGImage& operator=(SVGImage&& other) noexcept;

    static std::shared_ptr<SVGImage> LoadFromFile(const std::string& path, float dpi = 96.0f);

    static std::shared_ptr<SVGImage> LoadFromMemory(const void* data, size_t data_size, float dpi = 96.0f);

    static std::shared_ptr<SVGImage> LoadFromURL(const std::string& url, float dpi = 96.0f);

    bool Rasterize(uint32_t width, uint32_t height);

    bool Rasterize(float scale = 1.0f);

    [[nodiscard]] float GetOriginalWidth() const { return m_OriginalWidth; }
    [[nodiscard]] float GetOriginalHeight() const { return m_OriginalHeight; }

    static void RenderSVG(const std::shared_ptr<SVGImage>& svg, ImVec2 pos, float scale);
    static void RenderSVG(const std::shared_ptr<SVGImage>& image, ImVec2 pos, ImVec2 size);
    static void RenderSVG(const std::shared_ptr<SVGImage>& image, ImVec2 pos, ImVec2 size, float opacity);


private:
    static NSVGimage* ParseSVGFromMemory(const void* data, size_t data_size, float dpi);

    std::vector<uint8_t> RasterizeSVG(uint32_t width, uint32_t height);

    NSVGimage* m_SVGImage = nullptr;

    float m_OriginalWidth = 0.0f;
    float m_OriginalHeight = 0.0f;
  };
}  // namespace Infinity
