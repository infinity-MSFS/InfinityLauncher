#include "Image.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <curl/curl.h>
#include <iostream>

#include "Backend/TextureQueue/TextureQueue.hpp"
#include "png.h"
#include "turbojpeg.h"
#include "webp/decode.h"
#include "webp/demux.h"

namespace Infinity {

  namespace Utils {
    size_t WriteImageCallback(void *contents, size_t size, size_t nmemb, void *userp) {
      auto *buffer = static_cast<std::vector<uint8_t> *>(userp);
      buffer->insert(buffer->end(), static_cast<uint8_t *>(contents), static_cast<uint8_t *>(contents) + size * nmemb);
      return size * nmemb;
    }

    const std::string FALLBACK_URL =
        "https://cdn.jsdelivr.net/gh/infinity-MSFS/assets@master/delta/h60_background.webp";
  }  // namespace Utils

  class Image::Impl {
public:
    std::vector<uint8_t> pixel_data;
    GLuint textureId = 0;
    void *imguiTextureId = nullptr;

    ~Impl() { Release(); }

    void Release() {
      if (textureId) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
        imguiTextureId = nullptr;
      }
    }
  };

  std::unordered_map<ImGuiID, float> Image::s_animation_progress;

  Image::Image()
      : m_impl(std::make_unique<Impl>()) {}

  Image::~Image() { Release(); }

  Image::Image(Image &&other) noexcept
      : m_width(other.m_width)
      , m_height(other.m_height)
      , m_format(other.m_format)
      , m_impl(std::move(other.m_impl)) {
    other.m_width = 0;
    other.m_height = 0;
    other.m_format = Format::None;
  }

  Image &Image::operator=(Image &&other) noexcept {
    if (this != &other) {
      Release();
      m_impl = std::move(other.m_impl);
      m_width = other.m_width;
      m_height = other.m_height;
      m_format = other.m_format;

      other.m_width = 0;
      other.m_height = 0;
      other.m_format = Format::None;
    }
    return *this;
  }

  void Image::CreateGLTexture() {
    if (!glfwGetCurrentContext()) {
      std::cerr << "Error: GLFW context is not set!" << std::endl;
      return;
    }
    if (m_impl->textureId == 0 && !m_impl->pixel_data.empty()) {
      AllocateMemory(m_impl->pixel_data.data());
      m_impl->pixel_data.clear();
    }
  }


  std::shared_ptr<Image> Image::Create(uint32_t width, uint32_t height, Format format, const void *data) {
    auto image = std::make_shared<Image>();
    image->m_width = width;
    image->m_height = height;
    image->m_format = format;
    image->AllocateMemory(data);
    return image;
  }

  std::shared_ptr<Image> Image::LoadFromMemory(const void *data, size_t dataSize) {
    if (!data || dataSize == 0) {
      std::cerr << "Invalid memory data for image loading" << std::endl;
      return nullptr;
    }

    uint32_t width, height;
    std::vector<uint8_t> decodedData = DecodeImage(static_cast<const uint8_t *>(data), dataSize, width, height);

    if (decodedData.empty()) {
      return nullptr;
    }

    auto image = std::make_shared<Image>();
    image->m_width = width;
    image->m_height = height;
    image->m_format = Format::RGBA8;

    image->m_impl->pixel_data = std::move(decodedData);
    {
      std::lock_guard<std::mutex> lock(g_texture_queue_mutex);
      g_texture_creation_queue.push_back(image);
    }

    return image;
  }

  std::shared_ptr<Image> Image::LoadFromURL(const std::string &url) {
    try {
      if (url.contains("discordapp.") && url != Utils::FALLBACK_URL) {
        std::cerr << "Found an image with a Discord link, replacing with dummy "
                     "image\n";
        return LoadFromURL(Utils::FALLBACK_URL);
      }

      const std::vector<uint8_t> buffer = FetchFromURL(url);
      if (buffer.empty()) {
        return nullptr;
      }

      return LoadFromBinary(buffer, url);
    } catch (const std::exception &e) {
      std::cerr << "Failed to load image from URL: " << url << " : " << e.what() << std::endl;
      return nullptr;
    }
  }

  std::shared_ptr<Image> Image::LoadFromBinary(const std::vector<uint8_t> &binaryData, const std::string &url_ref) {
    if (binaryData.empty()) {
      return nullptr;
    }

    uint32_t width, height;
    std::vector<uint8_t> decodedData = DecodeImage(binaryData.data(), binaryData.size(), width, height, url_ref);

    if (decodedData.empty()) {
      return nullptr;
    }

    auto image = std::make_shared<Image>();
    image->m_width = width;
    image->m_height = height;
    image->m_format = Format::RGBA8;

    image->m_impl->pixel_data = std::move(decodedData);


    {
      std::lock_guard lock(g_texture_queue_mutex);
      g_texture_creation_queue.push_back(image);
    }

    return image;
  }

  std::vector<uint8_t> Image::FetchFromURL(const std::string &url) {
    CURL *curl = curl_easy_init();
    if (!curl) {
      std::cerr << "curl_easy_init failed" << std::endl;
      return {};
    }

    std::vector<uint8_t> buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Utils::WriteImageCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    const CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      std::cerr << "Failed to download image: " << curl_easy_strerror(res) << std::endl;
      return {};
    }

    return buffer;
  }

  std::vector<uint8_t> Image::DecodeImage(const uint8_t *data, size_t dataSize, uint32_t &outWidth, uint32_t &outHeight,
                                          const std::string &url_ref) {
    auto is_jpeg = [](const uint8_t *d) { return d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF; };

    auto is_png = [](const uint8_t *d) { return memcmp(d, "\x89PNG\r\n\x1A\n", 8) == 0; };

    auto decode_jpeg = [&](const uint8_t *d, size_t size) -> std::vector<uint8_t> {
      tjhandle handle = tjInitDecompress();
      if (!handle) return {};

      int width, height, subsamp, colorspace;
      if (tjDecompressHeader3(handle, d, size, &width, &height, &subsamp, &colorspace) != 0) {
        tjDestroy(handle);
        return {};
      }

      std::vector<uint8_t> buffer(width * height * 4);  // RGBA

      if (tjDecompress2(handle, d, size, buffer.data(), width, 0, height, TJPF_RGBA, TJFLAG_FASTDCT) != 0) {
        tjDestroy(handle);
        return {};
      }

      tjDestroy(handle);
      outWidth = width;
      outHeight = height;
      return buffer;
    };

    auto decode_png = [&](const uint8_t *d, size_t size) -> std::vector<uint8_t> {
      png_image image{};
      image.version = PNG_IMAGE_VERSION;

      if (!png_image_begin_read_from_memory(&image, d, size)) {
        return {};
      }

      image.format = PNG_FORMAT_RGBA;
      std::vector<uint8_t> buffer(PNG_IMAGE_SIZE(image));
      if (buffer.empty()) return {};

      if (!png_image_finish_read(&image, nullptr, buffer.data(), 0, nullptr)) {
        return {};
      }

      outWidth = image.width;
      outHeight = image.height;
      return buffer;
    };

    if (dataSize >= 3 && is_jpeg(data)) {
      auto result = decode_jpeg(data, dataSize);
      if (!result.empty()) return result;
    }

    if (dataSize >= 8 && is_png(data)) {
      auto result = decode_png(data, dataSize);
      if (!result.empty()) return result;
    }

    int width = 0, height = 0;
    if (WebPGetInfo(data, dataSize, &width, &height)) {
      uint8_t *webpRaw = WebPDecodeRGBA(data, dataSize, &width, &height);
      if (webpRaw) {
        outWidth = static_cast<uint32_t>(width);
        outHeight = static_cast<uint32_t>(height);
        std::vector<uint8_t> result(webpRaw, webpRaw + (width * height * 4));
        WebPFree(webpRaw);
        return result;
      }
    }

    std::cerr << "Failed to decode image: unsupported or corrupt format, URL: " << url_ref << std::endl;
    return {};
  }

  void Image::AllocateMemory(const void *data) const {
    if (!glfwGetCurrentContext()) {
      std::cerr << "glfwGetCurrentContext failed" << std::endl;
      return;
    }

    if (m_impl->textureId) {
      Release();
    }

    glGenTextures(1, &m_impl->textureId);
    if (m_impl->textureId == 0) {
      std::cerr << "glGenTextures failed!" << std::endl;
      if (const GLenum err = glGetError(); err != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << err << std::endl;
      }
      return;
    }
    glBindTexture(GL_TEXTURE_2D, m_impl->textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    const GLenum format = GetGLFormat(m_format);
    const GLenum internal_format = GetGLInternalFormat(m_format);
    const GLenum type = GetGLDataType(m_format);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal_format), static_cast<int>(m_width),
                 static_cast<int>(m_height), 0, format, type, data);

    m_impl->imguiTextureId = reinterpret_cast<void *>(static_cast<uintptr_t>(m_impl->textureId));
    std::cout << "Allocated texture with ID: " << m_impl->textureId << std::endl;

    if (const GLenum err = glGetError(); err != GL_NO_ERROR) {
      std::cerr << "OpenGL error during texture allocation: " << err << std::endl;
    }
  }

  void Image::SetData(const void *data) const {
    if (!data || !m_impl->textureId) return;

    glBindTexture(GL_TEXTURE_2D, m_impl->textureId);

    const GLenum format = GetGLFormat(m_format);
    const GLenum type = GetGLDataType(m_format);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<int>(m_width), static_cast<int>(m_height), format, type, data);
  }

  void Image::Resize(const uint32_t width, const uint32_t height) {
    if (m_impl->textureId && m_width == width && m_height == height) return;

    m_width = width;
    m_height = height;

    Release();
    AllocateMemory(nullptr);
  }

  void Image::Release() const { m_impl->Release(); }

  uint32_t Image::GetTextureID() const { return m_impl->textureId; }

  void *Image::GetImGuiTextureID() const { return m_impl->imguiTextureId; }

  float &Image::GetAnimationProgress(const ImGuiID id) { return s_animation_progress[id]; }

  uint32_t Image::GetGLFormat(const Format format) {
    switch (format) {
      case Format::RGBA8:
      case Format::RGBA32F:
        return GL_RGBA;
      case Format::None:
      default:
        return GL_NONE;
    }
  }

  uint32_t Image::GetGLInternalFormat(const Format format) {
    switch (format) {
      case Format::RGBA8:
        return GL_RGBA8;
      case Format::RGBA32F:
        return GL_RGBA32F;
      case Format::None:
      default:
        return GL_NONE;
    }
  }

  uint32_t Image::GetGLDataType(const Format format) {
    switch (format) {
      case Format::RGBA8:
        return GL_UNSIGNED_BYTE;
      case Format::RGBA32F:
        return GL_FLOAT;
      case Format::None:
      default:
        return GL_NONE;
    }
  }

  uint32_t Image::GetBytesPerPixel(const Format format) {
    switch (format) {
      case Format::RGBA8:
        return 4;
      case Format::RGBA32F:
        return 16;
      case Format::None:
      default:
        return 0;
    }
  }

  void Image::RenderImage(const std::unique_ptr<Image> &image, const ImVec2 pos, const float scale) {
    if (!image) return;
    ImGui::GetWindowDrawList()->AddImage(image->GetImGuiTextureID(), pos,
                                         {pos.x + image->GetWidth() * scale, pos.y + image->GetHeight() * scale});
  }

  void Image::RenderImage(const std::shared_ptr<Image> &image, const ImVec2 pos, const float scale) {
    if (!image) return;
    ImGui::GetWindowDrawList()->AddImage(image->GetImGuiTextureID(), pos,
                                         {pos.x + image->GetWidth() * scale, pos.y + image->GetHeight() * scale});
  }

  void Image::RenderImage(const std::unique_ptr<Image> &image, const ImVec2 pos, const ImVec2 size) {
    if (!image) return;

    const auto imgWidth = static_cast<float>(image->GetWidth());
    const auto imgHeight = static_cast<float>(image->GetHeight());

    auto getProperWidth = [](const ImVec2 &original_size, ImVec2 &new_size) {
      const float aspect_ratio = original_size.x / original_size.y;
      new_size.x = new_size.y * aspect_ratio;
    };

    const ImVec2 image_size(imgWidth, imgHeight);
    ImVec2 rescaled_image_size(0.0f, size.y);
    getProperWidth(image_size, rescaled_image_size);

    const float aspect_shown = rescaled_image_size.x / size.x;
    const float uv_x = 1.0f / aspect_shown;

    ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void *>(static_cast<intptr_t>(image->GetTextureID())), pos,
                                         {pos.x + size.x, pos.y + size.y}, {0.5f - uv_x / 2.0f, 0.0f},
                                         {0.5f + uv_x / 2.0f, 1.0f});
  }


  void Image::RenderImage(const std::shared_ptr<Image> &image, const ImVec2 pos, const ImVec2 size) {
    if (!image) return;

    const auto imgWidth = static_cast<float>(image->GetWidth());
    const auto imgHeight = static_cast<float>(image->GetHeight());

    auto getProperWidth = [](const ImVec2 &original_size, ImVec2 &new_size) {
      const float aspect_ratio = original_size.x / original_size.y;
      new_size.x = new_size.y * aspect_ratio;
    };

    const ImVec2 image_size(imgWidth, imgHeight);
    ImVec2 rescaled_image_size(0.0f, size.y);
    getProperWidth(image_size, rescaled_image_size);

    const float aspect_shown = rescaled_image_size.x / size.x;
    const float uv_x = 1.0f / aspect_shown;

    ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void *>(static_cast<intptr_t>(image->GetTextureID())), pos,
                                         {pos.x + size.x, pos.y + size.y}, {0.5f - uv_x / 2.0f, 0.0f},
                                         {0.5f + uv_x / 2.0f, 1.0f});
  }


  void Image::RenderHomeImage(const std::unique_ptr<Image> &image, const ImVec2 pos, const ImVec2 size,
                              bool is_hovered) {
    if (!image) return;

    ImGuiID id = ImGui::GetID(std::to_string(pos.x + pos.y).c_str());

    if (!s_animation_progress.contains(id)) {
      s_animation_progress[id] = 0.0f;
    }

    const auto imgWidth = static_cast<float>(image->GetWidth());
    const auto imgHeight = static_cast<float>(image->GetHeight());

    auto getProperWidth = [](const ImVec2 &original_size, ImVec2 &new_size) {
      const float aspect_ratio = original_size.x / original_size.y;
      new_size.x = new_size.y * aspect_ratio;
    };

    const ImVec2 image_size(imgWidth, imgHeight);
    ImVec2 rescaled_image_size(0.0f, size.y);
    getProperWidth(image_size, rescaled_image_size);

    const float aspect_shown = rescaled_image_size.x / size.x;
    const float base_uv_x = 1.0f / aspect_shown;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    const float animation_speed = 3.1f;
    const float hover_opacity_boost = 0.3f;
    const float zoom_factor = -0.01f;

#ifdef WIN32
    if (is_hovered) {
      s_AnimationProgress[id] = min(1.0f, s_AnimationProgress[id] + ImGui::GetIO().DeltaTime * animation_speed);
    } else {
      s_AnimationProgress[id] = max(0.0f, s_AnimationProgress[id] - ImGui::GetIO().DeltaTime * animation_speed);
    }
#else
    if (is_hovered) {
      s_animation_progress[id] = std::min(1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
    } else {
      s_animation_progress[id] = std::max(0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
    }
#endif


    const int segments = 60;

    const float segment_height = size.y / segments;
    const float uv_segment_height = 1.0f / segments;

    float zoom_amount = zoom_factor * s_animation_progress[id];
    float uv_x = base_uv_x * (1.0f + zoom_amount);

    const float uv_left = 0.5f - uv_x / 2.0f;
    const float uv_right = 0.5f + uv_x / 2.0f;

    for (int i = 0; i < segments; i++) {
      float y_start = pos.y + (i * segment_height);
      float y_end = y_start + segment_height;

      float base_uv_y_start = i * uv_segment_height;
      float base_uv_y_end = (i + 1) * uv_segment_height;

      float uv_y_center = 0.5f;
      float uv_y_start = uv_y_center + (base_uv_y_start - uv_y_center) * (1.0f + zoom_amount);
      float uv_y_end = uv_y_center + (base_uv_y_end - uv_y_center) * (1.0f + zoom_amount);

      float base_alpha = (float) i / segments;

      float hover_boost = hover_opacity_boost * s_animation_progress[id];
#ifdef WIN32
      float alpha = min(1.0f, base_alpha + hover_boost);
#else
      float alpha = std::min(1.0f, base_alpha + hover_boost);
#endif
      ImVec4 tint_color(1.0f, 1.0f, 1.0f, alpha);

      draw_list->AddImage((void *) (intptr_t) image->GetTextureID(), ImVec2(pos.x, y_start),
                          ImVec2(pos.x + size.x, y_end), ImVec2(uv_left, uv_y_start), ImVec2(uv_right, uv_y_end),
                          ImGui::ColorConvertFloat4ToU32(tint_color));
    }
  }

  void Image::RenderHomeImage(const std::shared_ptr<Image> &image, const ImVec2 pos, const ImVec2 size,
                              bool is_hovered) {
    if (!image) return;

    ImGuiID id = ImGui::GetID(std::to_string(pos.x + pos.y).c_str());

    if (s_animation_progress.find(id) == s_animation_progress.end()) {
      s_animation_progress[id] = 0.0f;
    }

    const float imgWidth = image->GetWidth();
    const float imgHeight = image->GetHeight();

    auto getProperWidth = [](const ImVec2 &original_size, ImVec2 &new_size) {
      const float aspect_ratio = original_size.x / original_size.y;
      new_size.x = new_size.y * aspect_ratio;
    };

    const ImVec2 image_size(imgWidth, imgHeight);
    ImVec2 rescaled_image_size(0.0f, size.y);
    getProperWidth(image_size, rescaled_image_size);

    const float aspect_shown = rescaled_image_size.x / size.x;
    const float base_uv_x = 1.0f / aspect_shown;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    const float animation_speed = 3.1f;
    const float hover_opacity_boost = 0.3f;
    const float zoom_factor = -0.01f;

#ifdef WIN32
    if (is_hovered) {
      s_AnimationProgress[id] = min(1.0f, s_AnimationProgress[id] + ImGui::GetIO().DeltaTime * animation_speed);
    } else {
      s_AnimationProgress[id] = max(0.0f, s_AnimationProgress[id] - ImGui::GetIO().DeltaTime * animation_speed);
    }
#else
    if (is_hovered) {
      s_animation_progress[id] = std::min(1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
    } else {
      s_animation_progress[id] = std::max(0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
    }
#endif

    const int segments = 60;

    const float segment_height = size.y / segments;
    const float uv_segment_height = 1.0f / segments;

    float zoom_amount = zoom_factor * s_animation_progress[id];
    float uv_x = base_uv_x * (1.0f + zoom_amount);

    const float uv_left = 0.5f - uv_x / 2.0f;
    const float uv_right = 0.5f + uv_x / 2.0f;

    for (int i = 0; i < segments; i++) {
      float y_start = pos.y + (i * segment_height);
      float y_end = y_start + segment_height;

      float base_uv_y_start = i * uv_segment_height;
      float base_uv_y_end = (i + 1) * uv_segment_height;

      float uv_y_center = 0.5f;
      float uv_y_start = uv_y_center + (base_uv_y_start - uv_y_center) * (1.0f + zoom_amount);
      float uv_y_end = uv_y_center + (base_uv_y_end - uv_y_center) * (1.0f + zoom_amount);

      float base_alpha = (float) i / segments;

      float hover_boost = hover_opacity_boost * s_animation_progress[id];
#ifdef WIN32
      float alpha = min(1.0f, base_alpha + hover_boost);
#else
      float alpha = std::min(1.0f, base_alpha + hover_boost);
#endif

      ImVec4 tint_color(1.0f, 1.0f, 1.0f, alpha);

      draw_list->AddImage((void *) (intptr_t) image->GetTextureID(), ImVec2(pos.x, y_start),
                          ImVec2(pos.x + size.x, y_end), ImVec2(uv_left, uv_y_start), ImVec2(uv_right, uv_y_end),
                          ImGui::ColorConvertFloat4ToU32(tint_color));
    }
  }

  void Image::RenderImage(const std::shared_ptr<Image> &image, ImVec2 pos, ImVec2 size, float opacity) {
    if (!image) return;

    const float imgWidth = image->GetWidth();
    const float imgHeight = image->GetHeight();

    float aspectRatio = imgWidth / imgHeight;

    float newHeight = size.x / aspectRatio;
    float newWidth = size.y * aspectRatio;

    float uvLeft = 0.0f, uvRight = 1.0f, uvTop = 0.0f, uvBottom = 1.0f;

    if (newHeight > size.y) {
      float excessHeight = newHeight - size.y;
      float uvCrop = (excessHeight / 2.0f) / newHeight;
      uvTop += uvCrop;
      uvBottom -= uvCrop;
      newHeight = size.y;
    } else if (newWidth > size.x) {
      float excessWidth = newWidth - size.x;
      float uvCrop = (excessWidth / 2.0f) / newWidth;
      uvLeft += uvCrop;
      uvRight -= uvCrop;
      newWidth = size.x;
    }

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    ImVec4 tint_color(1.0f, 1.0f, 1.0f, opacity);
    draw_list->AddImage((void *) (intptr_t) image->GetTextureID(), ImVec2(pos.x, pos.y),
                        ImVec2(pos.x + size.x, pos.y + size.y), ImVec2(uvLeft, uvTop), ImVec2(uvRight, uvBottom),
                        ImGui::ColorConvertFloat4ToU32(tint_color));
  }

}  // namespace Infinity
