#include "Image.hpp"

#include <algorithm>
#include <curl/curl.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/glfw3.h>

#include "Backend/TextureQueue/TextureQueue.hpp"
#include "stb_image/stb_image.h"
#include "webp/decode.h"

namespace Infinity {

  namespace Utils {
    size_t WriteImageCallback(void *contents, size_t size, size_t nmemb,
                              void *userp) {
      auto *buffer = static_cast<std::vector<uint8_t> *>(userp);
      buffer->insert(buffer->end(), static_cast<uint8_t *>(contents),
                     static_cast<uint8_t *>(contents) + size * nmemb);
      return size * nmemb;
    }

    const std::string FALLBACK_URL =
        "https://1000logos.net/wp-content/uploads/2021/06/Discord-logo.png";
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

  // Static member initialization
  std::unordered_map<ImGuiID, float> Image::s_AnimationProgress;

  // Image implementation
  Image::Image()
      : m_Impl(std::make_unique<Impl>()) {}

  Image::~Image() { Release(); }

  Image::Image(Image &&other) noexcept
      : m_Impl(std::move(other.m_Impl))
      , m_Width(other.m_Width)
      , m_Height(other.m_Height)
      , m_Format(other.m_Format) {
    other.m_Width = 0;
    other.m_Height = 0;
    other.m_Format = Format::None;
  }

  Image &Image::operator=(Image &&other) noexcept {
    if (this != &other) {
      Release();
      m_Impl = std::move(other.m_Impl);
      m_Width = other.m_Width;
      m_Height = other.m_Height;
      m_Format = other.m_Format;

      other.m_Width = 0;
      other.m_Height = 0;
      other.m_Format = Format::None;
    }
    return *this;
  }

  void Image::CreateGLTexture() {
    if (!glfwGetCurrentContext()) {
      std::cerr << "Error: GLFW context is not set!" << std::endl;
      return;
    }
    if (m_Impl->textureId == 0 && !m_Impl->pixel_data.empty()) {
      AllocateMemory(m_Impl->pixel_data.data());
      m_Impl->pixel_data.clear();
    }
  }


  std::shared_ptr<Image> Image::Create(uint32_t width, uint32_t height,
                                       Format format, const void *data) {
    auto image = std::make_shared<Image>();
    image->m_Width = width;
    image->m_Height = height;
    image->m_Format = format;
    image->AllocateMemory(data);
    return image;
  }

  std::shared_ptr<Image> Image::LoadFromMemory(const void *data,
                                               size_t dataSize) {
    if (!data || dataSize == 0) {
      std::cerr << "Invalid memory data for image loading" << std::endl;
      return nullptr;
    }

    uint32_t width, height;
    void *decodedData = DecodeImage(static_cast<const uint8_t *>(data),
                                    dataSize, width, height);

    if (!decodedData) {
      return nullptr;
    }

    auto image = std::make_shared<Image>();
    image->m_Width = width;
    image->m_Height = height;
    image->m_Format = Format::RGBA8;

    image->m_Impl->pixel_data.assign(
        static_cast<uint8_t *>(decodedData),
        static_cast<uint8_t *>(decodedData) + (width * height * 4));

    stbi_image_free(decodedData);

    {
      std::lock_guard<std::mutex> lock(g_TextureQueueMutex);
      g_TextureCreationQueue.push_back(image);
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

      std::vector<uint8_t> buffer = FetchFromURL(url);
      if (buffer.empty()) {
        return nullptr;
      }

      return LoadFromBinary(buffer);
    } catch (const std::exception &e) {
      std::cerr << "Failed to load image from URL: " << url << " : " << e.what()
                << std::endl;
      return nullptr;
    }
  }

  std::shared_ptr<Image> Image::LoadFromBinary(
      const std::vector<uint8_t> &binaryData) {
    if (binaryData.empty()) {
      return nullptr;
    }

    uint32_t width, height;
    void *decodedData =
        DecodeImage(binaryData.data(), binaryData.size(), width, height);

    if (!decodedData) {
      return nullptr;
    }

    auto image = std::make_shared<Image>();
    image->m_Width = width;
    image->m_Height = height;
    image->m_Format = Format::RGBA8;

    image->m_Impl->pixel_data.assign(
        static_cast<uint8_t *>(decodedData),
        static_cast<uint8_t *>(decodedData) + (width * height * 4));

    stbi_image_free(decodedData);

    {
      std::lock_guard<std::mutex> lock(g_TextureQueueMutex);
      g_TextureCreationQueue.push_back(image);
    }

    return image;
  }

  std::vector<uint8_t> Image::FetchFromURL(const std::string &url) {
    // Initialize curl
    CURL *curl = curl_easy_init();
    if (!curl) {
      std::cerr << "curl_easy_init failed" << std::endl;
      return {};
    }

    // Setup curl
    std::vector<uint8_t> buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Utils::WriteImageCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      std::cerr << "Failed to download image: " << curl_easy_strerror(res)
                << std::endl;
      return {};
    }

    return buffer;
  }

  void *Image::DecodeImage(const uint8_t *data, size_t dataSize,
                           uint32_t &outWidth, uint32_t &outHeight) {
    // Try to decode with stb_image first
    int width, height, channels;
    uint8_t *decodedData =
        stbi_load_from_memory(data, static_cast<int>(dataSize), &width, &height,
                              &channels, STBI_rgb_alpha);

    if (decodedData) {
      outWidth = static_cast<uint32_t>(width);
      outHeight = static_cast<uint32_t>(height);
      return decodedData;
    }

    // If stb_image failed, try WebP
    width = 0;  // Reset in case stb_image modified these
    height = 0;

    if (WebPGetInfo(data, dataSize, &width, &height)) {
      outWidth = static_cast<uint32_t>(width);
      outHeight = static_cast<uint32_t>(height);
      uint8_t *webpData = WebPDecodeRGBA(data, dataSize, &width, &height);

      if (webpData) {
        return webpData;
      }
    }

    std::cerr << "Failed to decode image data: " << stbi_failure_reason()
              << std::endl;
    return nullptr;
  }

  void Image::AllocateMemory(const void *data) {
    if (!glfwGetCurrentContext()) {
      std::cerr << "glfwGetCurrentContext failed" << std::endl;
      return;
    }

    // If we already have a texture, release it
    if (m_Impl->textureId) {
      Release();
    }

    // Generate a new texture
    glGenTextures(1, &m_Impl->textureId);
    if (m_Impl->textureId == 0) {
      std::cerr << "glGenTextures failed!" << std::endl;
      GLenum err = glGetError();
      if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << err << std::endl;
      }
      return;
    }
    glBindTexture(GL_TEXTURE_2D, m_Impl->textureId);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set pixel storage alignment
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    // Upload texture data
    GLenum format = GetGLFormat(m_Format);
    GLenum internalFormat = GetGLInternalFormat(m_Format);
    GLenum type = GetGLDataType(m_Format);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format,
                 type, data);

    // Store ImGui texture ID
    m_Impl->imguiTextureId =
        reinterpret_cast<void *>(static_cast<uintptr_t>(m_Impl->textureId));
    std::cout << "Allocated texture with ID: " << m_Impl->textureId
              << std::endl;

    // Check for errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
      std::cerr << "OpenGL error during texture allocation: " << err
                << std::endl;
    }
  }

  void Image::SetData(const void *data) {
    if (!data || !m_Impl->textureId) return;

    glBindTexture(GL_TEXTURE_2D, m_Impl->textureId);

    GLenum format = GetGLFormat(m_Format);
    GLenum type = GetGLDataType(m_Format);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, format, type,
                    data);
  }

  void Image::Resize(uint32_t width, uint32_t height) {
    // Skip if dimensions are the same
    if (m_Impl->textureId && m_Width == width && m_Height == height) return;

    m_Width = width;
    m_Height = height;

    // Reallocate with new dimensions
    Release();
    AllocateMemory(nullptr);  // Allocate empty texture with new dimensions
  }

  void Image::Release() { m_Impl->Release(); }

  uint32_t Image::GetTextureID() const { return m_Impl->textureId; }

  void *Image::GetImGuiTextureID() const { return m_Impl->imguiTextureId; }

  float &Image::GetAnimationProgress(ImGuiID id) {
    return s_AnimationProgress[id];
  }

  uint32_t Image::GetGLFormat(Format format) {
    switch (format) {
      case Format::RGBA8:
      case Format::RGBA32F:
        return GL_RGBA;
      case Format::None:
      default:
        return GL_NONE;
    }
  }

  uint32_t Image::GetGLInternalFormat(Format format) {
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

  uint32_t Image::GetGLDataType(Format format) {
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

  uint32_t Image::GetBytesPerPixel(Format format) {
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

  void Image::RenderImage(const std::unique_ptr<Image> &image, const ImVec2 pos,
                          const float scale) {
    if (!image) return;
    ImGui::GetWindowDrawList()->AddImage(image->GetImGuiTextureID(), pos,
                                         {pos.x + image->GetWidth() * scale,
                                          pos.y + image->GetHeight() * scale});
  }

  void Image::RenderImage(const std::shared_ptr<Image> &image, const ImVec2 pos,
                          const float scale) {
    if (!image) return;
    ImGui::GetWindowDrawList()->AddImage(image->GetImGuiTextureID(), pos,
                                         {pos.x + image->GetWidth() * scale,
                                          pos.y + image->GetHeight() * scale});
  }

  void Image::RenderImage(const std::unique_ptr<Image> &image, const ImVec2 pos,
                          const ImVec2 size) {
    if (!image) return;

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
    const float uv_x = 1.0f / aspect_shown;

    ImGui::GetWindowDrawList()->AddImage(
        (void *) (intptr_t) image->GetTextureID(), pos,
        {pos.x + size.x, pos.y + size.y}, {0.5f - uv_x / 2.0f, 0.0f},
        {0.5f + uv_x / 2.0f, 1.0f});
  }


  void Image::RenderImage(const std::shared_ptr<Image> &image, const ImVec2 pos,
                          const ImVec2 size) {
    if (!image) return;

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
    const float uv_x = 1.0f / aspect_shown;

    ImGui::GetWindowDrawList()->AddImage(
        (void *) (intptr_t) image->GetTextureID(), pos,
        {pos.x + size.x, pos.y + size.y}, {0.5f - uv_x / 2.0f, 0.0f},
        {0.5f + uv_x / 2.0f, 1.0f});
  }


  void Image::RenderHomeImage(const std::unique_ptr<Image> &image,
                              const ImVec2 pos, const ImVec2 size,
                              bool is_hovered) {
    if (!image) return;

    ImGuiID id = ImGui::GetID(std::to_string(pos.x + pos.y).c_str());

    if (s_AnimationProgress.find(id) == s_AnimationProgress.end()) {
      s_AnimationProgress[id] = 0.0f;
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
      s_AnimationProgress[id] = min(
          1.0f,
          s_AnimationProgress[id] + ImGui::GetIO().DeltaTime * animation_speed);
    } else {
      s_AnimationProgress[id] = max(
          0.0f,
          s_AnimationProgress[id] - ImGui::GetIO().DeltaTime * animation_speed);
    }
#else
    if (is_hovered) {
      s_AnimationProgress[id] = std::min(
          1.0f,
          s_AnimationProgress[id] + ImGui::GetIO().DeltaTime * animation_speed);
    } else {
      s_AnimationProgress[id] = std::max(
          0.0f,
          s_AnimationProgress[id] - ImGui::GetIO().DeltaTime * animation_speed);
    }
#endif


    const int segments = 60;

    const float segment_height = size.y / segments;
    const float uv_segment_height = 1.0f / segments;

    float zoom_amount = zoom_factor * s_AnimationProgress[id];
    float uv_x = base_uv_x * (1.0f + zoom_amount);

    const float uv_left = 0.5f - uv_x / 2.0f;
    const float uv_right = 0.5f + uv_x / 2.0f;

    for (int i = 0; i < segments; i++) {
      float y_start = pos.y + (i * segment_height);
      float y_end = y_start + segment_height;

      float base_uv_y_start = i * uv_segment_height;
      float base_uv_y_end = (i + 1) * uv_segment_height;

      float uv_y_center = 0.5f;
      float uv_y_start =
          uv_y_center + (base_uv_y_start - uv_y_center) * (1.0f + zoom_amount);
      float uv_y_end =
          uv_y_center + (base_uv_y_end - uv_y_center) * (1.0f + zoom_amount);

      float base_alpha = (float) i / segments;

      float hover_boost = hover_opacity_boost * s_AnimationProgress[id];
#ifdef WIN32
      float alpha = min(1.0f, base_alpha + hover_boost);
#else
      float alpha = std::min(1.0f, base_alpha + hover_boost);
#endif
      ImVec4 tint_color(1.0f, 1.0f, 1.0f, alpha);

      draw_list->AddImage((void *) (intptr_t) image->GetTextureID(),
                          ImVec2(pos.x, y_start), ImVec2(pos.x + size.x, y_end),
                          ImVec2(uv_left, uv_y_start),
                          ImVec2(uv_right, uv_y_end),
                          ImGui::ColorConvertFloat4ToU32(tint_color));
    }
  }

  void Image::RenderHomeImage(const std::shared_ptr<Image> &image,
                              const ImVec2 pos, const ImVec2 size,
                              bool is_hovered) {
    if (!image) return;

    ImGuiID id = ImGui::GetID(std::to_string(pos.x + pos.y).c_str());

    if (s_AnimationProgress.find(id) == s_AnimationProgress.end()) {
      s_AnimationProgress[id] = 0.0f;
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
      s_AnimationProgress[id] = min(
          1.0f,
          s_AnimationProgress[id] + ImGui::GetIO().DeltaTime * animation_speed);
    } else {
      s_AnimationProgress[id] = max(
          0.0f,
          s_AnimationProgress[id] - ImGui::GetIO().DeltaTime * animation_speed);
    }
#else
    if (is_hovered) {
      s_AnimationProgress[id] = std::min(
          1.0f,
          s_AnimationProgress[id] + ImGui::GetIO().DeltaTime * animation_speed);
    } else {
      s_AnimationProgress[id] = std::max(
          0.0f,
          s_AnimationProgress[id] - ImGui::GetIO().DeltaTime * animation_speed);
    }
#endif

    const int segments = 60;

    const float segment_height = size.y / segments;
    const float uv_segment_height = 1.0f / segments;

    float zoom_amount = zoom_factor * s_AnimationProgress[id];
    float uv_x = base_uv_x * (1.0f + zoom_amount);

    const float uv_left = 0.5f - uv_x / 2.0f;
    const float uv_right = 0.5f + uv_x / 2.0f;

    for (int i = 0; i < segments; i++) {
      float y_start = pos.y + (i * segment_height);
      float y_end = y_start + segment_height;

      float base_uv_y_start = i * uv_segment_height;
      float base_uv_y_end = (i + 1) * uv_segment_height;

      float uv_y_center = 0.5f;
      float uv_y_start =
          uv_y_center + (base_uv_y_start - uv_y_center) * (1.0f + zoom_amount);
      float uv_y_end =
          uv_y_center + (base_uv_y_end - uv_y_center) * (1.0f + zoom_amount);

      float base_alpha = (float) i / segments;

      float hover_boost = hover_opacity_boost * s_AnimationProgress[id];
#ifdef WIN32
      float alpha = min(1.0f, base_alpha + hover_boost);
#else
      float alpha = std::min(1.0f, base_alpha + hover_boost);
#endif

      ImVec4 tint_color(1.0f, 1.0f, 1.0f, alpha);

      draw_list->AddImage((void *) (intptr_t) image->GetTextureID(),
                          ImVec2(pos.x, y_start), ImVec2(pos.x + size.x, y_end),
                          ImVec2(uv_left, uv_y_start),
                          ImVec2(uv_right, uv_y_end),
                          ImGui::ColorConvertFloat4ToU32(tint_color));
    }
  }

  void Image::RenderImage(const std::shared_ptr<Image> &image, ImVec2 pos,
                          ImVec2 size, float opacity) {
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
    draw_list->AddImage(
        (void *) (intptr_t) image->GetTextureID(), ImVec2(pos.x, pos.y),
        ImVec2(pos.x + size.x, pos.y + size.y), ImVec2(uvLeft, uvTop),
        ImVec2(uvRight, uvBottom), ImGui::ColorConvertFloat4ToU32(tint_color));
  }

}  // namespace Infinity
