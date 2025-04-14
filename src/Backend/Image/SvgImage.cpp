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
  SVGImage::SVGImage() {}

  SVGImage::~SVGImage() {
    if (m_svg_image) {
      nsvgDelete(m_svg_image);
      m_svg_image = nullptr;
    }
  }

  SVGImage::SVGImage(SVGImage&& other) noexcept
      : Image(std::move(other))
      , m_svg_image(other.m_svg_image)
      , m_original_width(other.m_original_width)
      , m_original_height(other.m_original_height) {
    other.m_svg_image = nullptr;
    other.m_original_width = 0.0f;
    other.m_original_height = 0.0f;
  }

  SVGImage& SVGImage::operator=(SVGImage&& other) noexcept {
    if (this != &other) {
      Image::operator=(std::move(other));

      if (m_svg_image) {
        nsvgDelete(m_svg_image);
      }

      m_svg_image = other.m_svg_image;
      m_original_width = other.m_original_width;
      m_original_height = other.m_original_height;

      other.m_svg_image = nullptr;
      other.m_original_width = 0.0f;
      other.m_original_height = 0.0f;
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
    image->m_svg_image = svg_image;
    image->m_original_width = svg_image->width;
    image->m_original_height = svg_image->height;

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
    image->m_svg_image = svg_image;
    image->m_original_width = svg_image->width;
    image->m_original_height = svg_image->height;

    image->Rasterize(1.0f);

    return image;
  }

  std::shared_ptr<SVGImage> SVGImage::LoadFromURL(const std::string& url, float dpi) {
    try {
      std::cout << "Loading SVG from URL: " << url << std::endl;
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
    if (!m_svg_image) {
      std::cerr << "SVGImage is null, cannot rasterize" << std::endl;
      return false;
    }

    std::vector<uint8_t> pixels = RasterizeSVG(width, height);
    if (pixels.empty()) {
      std::cerr << "Failed to rasterize SVG image" << std::endl;
      return false;
    }

    m_width = width;
    m_height = height;
    m_format = Format::RGBA8;


    AllocateMemory(pixels.data());

    return true;
  }

  bool SVGImage::Rasterize(float scale) {
    if (!m_svg_image) {
      std::cerr << "SVGImage is null, cannot rasterize" << std::endl;
      return false;
    }

    auto width = static_cast<uint32_t>(m_original_width * scale);
    auto height = static_cast<uint32_t>(m_original_height * scale);

    return Rasterize(width, height);
  }

  std::vector<uint8_t> SVGImage::RasterizeSVG(const uint32_t width, const uint32_t height) const {
    if (!m_svg_image) {
      return {};
    }

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
      std::cerr << "Failed to create rasterizer" << std::endl;
      return {};
    }

    std::vector<uint8_t> pixels(width * height * 4, 0);

    float scale_x = width / m_svg_image->width;
    float scale_y = height / m_svg_image->height;
#ifdef WIN32
    float scale = min(scale_x, scale_y);
#else
    float scale = std::min(scale_x, scale_y);
#endif

    nsvgRasterize(rast, m_svg_image, 0, 0, scale, pixels.data(), width, height, width * 4);

    nsvgDeleteRasterizer(rast);

    std::cout << pixels.size() << std::endl;
    return pixels;
  }

  // a really shitty deep copy method (this will leak if it fails)
  std::shared_ptr<SVGImage> SVGImage::Clone() const {
    if (!m_svg_image) {
      return nullptr;
    }

    auto clone = std::make_shared<SVGImage>();

    auto cloned_svg = std::unique_ptr<NSVGimage, decltype(&nsvgDelete)>(
        static_cast<NSVGimage*>(calloc(1, sizeof(NSVGimage))), nsvgDelete);

    if (!cloned_svg) {
      return nullptr;
    }

    cloned_svg->width = m_svg_image->width;
    cloned_svg->height = m_svg_image->height;

    NSVGshape* lastShape = nullptr;

    for (NSVGshape* shape = m_svg_image->shapes; shape != nullptr; shape = shape->next) {
      struct ShapeDeleter {
        void operator()(NSVGshape* shape) const {
          if (!shape) return;

          NSVGpath* path = shape->paths;
          while (path) {
            NSVGpath* next = path->next;
            free(path->pts);
            free(path);
            path = next;
          }

          free(shape);
        }
      };

      std::unique_ptr<NSVGshape, ShapeDeleter> newShape(static_cast<NSVGshape*>(calloc(1, sizeof(NSVGshape))),
                                                        ShapeDeleter());

      if (!newShape) {
        return nullptr;
      }

      newShape->id[0] = '\0';
      if (shape->id[0] != '\0') {
        strncpy(newShape->id, shape->id, 63);
        newShape->id[63] = '\0';
      }

      newShape->fill = shape->fill;
      newShape->stroke = shape->stroke;
      newShape->opacity = shape->opacity;
      newShape->strokeWidth = shape->strokeWidth;
      newShape->strokeDashOffset = shape->strokeDashOffset;
      for (int i = 0; i < 8; i++) {
        newShape->strokeDashArray[i] = shape->strokeDashArray[i];
      }
      newShape->strokeDashCount = shape->strokeDashCount;
      newShape->strokeLineJoin = shape->strokeLineJoin;
      newShape->strokeLineCap = shape->strokeLineCap;
      newShape->fillRule = shape->fillRule;
      newShape->flags = shape->flags;

      NSVGpath* lastPath = nullptr;

      for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
        auto* newPath = static_cast<NSVGpath*>(calloc(1, sizeof(NSVGpath)));
        if (!newPath) {
          return nullptr;
        }

        newPath->closed = path->closed;
        newPath->npts = path->npts;

        if (path->npts > 0) {
          newPath->pts = static_cast<float*>(malloc(path->npts * 2 * sizeof(float)));
          if (!newPath->pts) {
            free(newPath);
            return nullptr;
          }
          memcpy(newPath->pts, path->pts, path->npts * 2 * sizeof(float));
        }

        if (lastPath == nullptr) {
          newShape->paths = newPath;
        } else {
          lastPath->next = newPath;
        }
        lastPath = newPath;
      }

      NSVGshape* rawShapePtr = newShape.release();

      if (lastShape == nullptr) {
        cloned_svg->shapes = rawShapePtr;
      } else {
        lastShape->next = rawShapePtr;
      }
      lastShape = rawShapePtr;
    }

    clone->m_svg_image = cloned_svg.release();
    clone->m_original_width = m_original_height;
    clone->m_original_height = m_original_height;

    if (m_width > 0 && m_height > 0) {
      clone->Rasterize(m_width, m_height);
    } else {
      clone->Rasterize(1.0f);
    }

    return clone;
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


  void SVGImage::ChangeColor(const std::string& color) {
    if (m_svg_image) return;

    if (color.empty() || (color[0] != '#' && color.length() != 7)) {
      std::cerr << "Invalid color format. Expected #RRGGBB\n";
    }

    int r, g, b;
    sscanf(color.c_str(), "%02x%02x%02x", &r, &g, &b);

    for (NSVGshape* shape = m_svg_image->shapes; shape != nullptr; shape = shape->next) {
      if (shape->fill.type != NSVG_PAINT_NONE) {
        shape->fill.color = (r << 16) | (g << 8) | b | (255 << 24);
      }

      if (shape->stroke.type != NSVG_PAINT_NONE) {
        shape->stroke.color = (r << 16) | (g << 8) | b | (255 << 24);
      }
    }
    Rasterize(1.0f);
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
