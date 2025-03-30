#include "SvgImage.hpp"


#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"

#define NANOSVGRAST_IMPLEMENTATION
#include <GL/glew.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

#include "nanosvgrast.h"
//
#include <GLFW/glfw3.h>

namespace Infinity {
  SVGImage::SVGImage()
      : Image()
      , m_SVGImage(nullptr)
      , m_OriginalWidth(0.0f)
      , m_OriginalHeight(0.0f) {}

  SVGImage::~SVGImage() {
    if (m_SVGImage) {
      nsvgDelete(m_SVGImage);
      m_SVGImage = nullptr;
    }
  }

  SVGImage::SVGImage(SVGImage&& other) noexcept
      : Image(std::move(other))
      , m_SVGImage(other.m_SVGImage)
      , m_OriginalWidth(other.m_OriginalWidth)
      , m_OriginalHeight(other.m_OriginalHeight) {
    other.m_SVGImage = nullptr;
    other.m_OriginalWidth = 0.0f;
    other.m_OriginalHeight = 0.0f;
  }

  SVGImage& SVGImage::operator=(SVGImage&& other) noexcept {
    if (this != &other) {
      Image::operator=(std::move(other));

      if (m_SVGImage) {
        nsvgDelete(m_SVGImage);
      }

      m_SVGImage = other.m_SVGImage;
      m_OriginalWidth = other.m_OriginalWidth;
      m_OriginalHeight = other.m_OriginalHeight;

      other.m_SVGImage = nullptr;
      other.m_OriginalWidth = 0.0f;
      other.m_OriginalHeight = 0.0f;
    }
    return *this;
  }

  std::shared_ptr<SVGImage> SVGImage::LoadFromFile(const std::string& path, float dpi) {
    NSVGimage* svg_image = nsvgParseFromFile(path.c_str(), "px", dpi);
    if (!svg_image) {
      std::cerr << "Error loading SVG file: " << path << std::endl;
      return nullptr;
    }

    auto image = std::make_shared<SVGImage>();
    image->m_SVGImage = svg_image;
    image->m_OriginalWidth = svg_image->width;
    image->m_OriginalHeight = svg_image->height;

    image->Rasterize(1.0f);

    return image;
  }

  std::shared_ptr<SVGImage> SVGImage::LoadFromMemory(const void* data, size_t data_size, float dpi) {
    NSVGimage* svg_image = ParseSVGFromMemory(data, data_size, dpi);
    if (!svg_image) {
      std::cerr << "Error loading SVG from memory" << std::endl;
      return nullptr;
    }

    auto image = std::make_shared<SVGImage>();
    image->m_SVGImage = svg_image;
    image->m_OriginalWidth = svg_image->width;
    image->m_OriginalHeight = svg_image->height;

    image->Rasterize(1.0f);

    return image;
  }

  std::shared_ptr<SVGImage> SVGImage::LoadFromURL(const std::string& url, float dpi) {
    try {
      std::vector<uint8_t> buffer = Image::FetchFromURL(url);
      if (buffer.empty()) {
        std::cerr << "Failed to fetch SVG from URL: " << url << std::endl;
        return nullptr;
      }
      return LoadFromMemory(buffer.data(), buffer.size(), dpi);
    } catch (const std::exception& e) {
      std::cerr << "Failed to load SVG from URL: " << url << " : " << e.what() << std::endl;
      return nullptr;
    }
  }

  NSVGimage* SVGImage::ParseSVGFromMemory(const void* data, size_t data_size, float dpi) {
    char* copy = new char[data_size + 1];
    std::memcpy(copy, data, data_size);
    copy[data_size] = '\0';

    NSVGimage* svg_image = nsvgParse(copy, "px", dpi);
    delete[] copy;

    return svg_image;
  }

  bool SVGImage::Rasterize(uint32_t width, uint32_t height) {
    if (!m_SVGImage) {
      std::cerr << "SVGImage is null, cannot rasterize" << std::endl;
      return false;
    }

    std::vector<uint8_t> pixels = RasterizeSVG(width, height);
    if (pixels.empty()) {
      std::cerr << "Failed to rasterize SVG image" << std::endl;
      return false;
    }

    m_Width = width;
    m_Height = height;
    m_Format = Format::RGBA8;


    AllocateMemory(pixels.data());

    return true;
  }

  bool SVGImage::Rasterize(float scale) {
    if (!m_SVGImage) {
      std::cerr << "SVGImage is null, cannot rasterize" << std::endl;
      return false;
    }

    uint32_t width = static_cast<uint32_t>(m_OriginalWidth * scale);
    uint32_t height = static_cast<uint32_t>(m_OriginalHeight * scale);

    return Rasterize(width, height);
  }

  std::vector<uint8_t> SVGImage::RasterizeSVG(uint32_t width, uint32_t height) {
    if (!m_SVGImage) {
      return {};
    }

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
      std::cerr << "Failed to create rasterizer" << std::endl;
      return {};
    }

    std::vector<uint8_t> pixels(width * height * 4, 0);

    float scale_x = width / m_SVGImage->width;
    float scale_y = height / m_SVGImage->height;
#ifdef WIN32
    float scale = min(scale_x, scale_y);
#else
    float scale = std::min(scale_x, scale_y);
#endif

    nsvgRasterize(rast, m_SVGImage, 0, 0, scale, pixels.data(), width, height, width * 4);

    nsvgDeleteRasterizer(rast);

    std::cout << pixels.size() << std::endl;
    return pixels;
  }

  void SVGImage::RenderSVG(const std::shared_ptr<SVGImage>& svg, ImVec2 pos, float scale) {
    if (!svg) return;

    if (scale != 1.0f) {
      svg->Rasterize(scale);
    }

    Image::RenderImage(svg, pos, scale);
  }

  void SVGImage::RenderSVG(const std::shared_ptr<SVGImage>& image, ImVec2 pos, ImVec2 size) {
    if (!image) return;

    float current_aspect = static_cast<float>(image->GetWidth()) / static_cast<float>(image->GetHeight());
    float requested_aspect = size.x / size.y;

    if (std::abs(current_aspect - requested_aspect) > 0.01f ||
        std::abs(static_cast<float>(image->GetWidth()) - size.x) > 2.0f ||
        std::abs(static_cast<float>(image->GetHeight()) - size.y) > 2.0f) {
      image->Rasterize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
    }

    Image::RenderImage(image, pos, size);
  }

  void SVGImage::RenderSVG(const std::shared_ptr<SVGImage>& image, ImVec2 pos, ImVec2 size, float opacity) {
    if (!image) return;

    float current_aspect = static_cast<float>(image->GetWidth()) / static_cast<float>(image->GetHeight());
    float requested_aspect = size.x / size.y;

    if (std::abs(current_aspect - requested_aspect) > 0.01f ||
        std::abs(static_cast<float>(image->GetWidth()) - size.x) > 2.0f ||
        std::abs(static_cast<float>(image->GetHeight()) - size.y) > 2.0f) {
      image->Rasterize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
    }

    Image::RenderImage(image, pos, size, opacity);
  }


}  // namespace Infinity
