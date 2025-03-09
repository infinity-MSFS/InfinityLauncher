#include "Image.hpp"
#include <algorithm>
#include <curl/curl.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <GL/gl.h>


#include "stb_image/stb_image.h"
#include "webp/decode.h"

namespace Infinity {
    namespace Utils {
        static uint32_t BytesPerPixel(ImageFormat format) {
            switch (format) {
                case ImageFormat::RGBA:
                    return 4;
                case ImageFormat::RGBA32F:
                    return 16;
                case ImageFormat::None:
                    return 0;
            }
            return 0;
        }

        static GLenum InfinityFormatToOpenGLFormat(ImageFormat format) {
            switch (format) {
                case ImageFormat::RGBA:
                    return GL_RGBA;
                case ImageFormat::RGBA32F:
                    return GL_RGBA32F;
                case ImageFormat::None:
                    return GL_NONE;
            }
            return GL_NONE;
        }
    } // namespace Utils

    class Image::Impl {
    public:
        GLuint texture_id = 0;
        ImTextureID descriptor_set = nullptr;

        ~Impl() { Release(); }

        void Release() {
            if (texture_id) {
                glDeleteTextures(1, &texture_id);
                texture_id = 0;
                descriptor_set = nullptr;
            }
        }
    };

    std::map<ImGuiID, float> Image::s_animation_progress;

    static size_t WriteImageCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        auto *buffer = static_cast<std::vector<uint8_t> *>(userp);
        size_t realSize = size * nmemb;
        buffer->insert(buffer->end(), static_cast<uint8_t *>(contents), static_cast<uint8_t *>(contents) + realSize);
        return realSize;
    }

    Image::Image(const uint32_t width, const uint32_t height, const ImageFormat format, const void *data) : m_Impl(std::make_unique<Impl>()), m_Width(width), m_Height(height), m_Format(format) {
        AllocateMemory(m_Width * m_Height * Utils::BytesPerPixel(m_Format));
        if (data) {
            SetData(data);
        }
    }

    Image::Image(const std::string &url) : m_Impl(std::make_unique<Impl>()), m_Format(ImageFormat::RGBA) {
        CURL *curl = curl_easy_init();
        if (!curl) {
            std::cerr << "curl_easy_init failed" << std::endl;
        }

        std::vector<uint8_t> buffer;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteImageCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            throw std::runtime_error("Failed to download image: " + std::string(curl_easy_strerror(res)));
        }

        uint32_t width, height;
        void *decodedData = Decode(buffer.data(), buffer.size(), width, height);

        if (!decodedData) {
            throw std::runtime_error("Failed to decode image data");
        }

        m_Width = width;
        m_Height = height;

        AllocateMemory(m_Width * m_Height * 4);
        SetData(decodedData);
        stbi_image_free(decodedData);
    }

    Image::Image(const std::vector<uint8_t> &bin) : m_Impl(std::make_unique<Impl>()), m_Format(ImageFormat::RGBA) {
        uint32_t width, height;
        void *decodedData = Decode(bin.data(), bin.size(), width, height);

        if (!decodedData) {
            throw std::runtime_error("Failed to decode image data");
        }

        m_Width = width;
        m_Height = height;

        AllocateMemory(m_Width * m_Height * 4);
        SetData(decodedData);
        stbi_image_free(decodedData);
    }

    std::shared_ptr<Image> Image::ConstructFromBin(const std::vector<uint8_t> &bin) {
        if (!bin.empty()) {
            return std::make_shared<Image>(bin);
        }
        return nullptr;
    }

    std::vector<uint8_t> Image::FetchFromURL(const std::string &url) {
        static const std::string fallback_url = "https://1000logos.net/wp-content/uploads/2021/06/Discord-logo.png";

        if (url.contains("discordapp.") && url != fallback_url) {
            std::cerr << "Found an image with a Discord link, replacing with dummy image\n";
            return FetchFromURL(fallback_url);
        }

        CURL *curl = curl_easy_init();
        if (!curl) {
            std::cerr << "curl_easy_init failed" << std::endl;
            return {};
        }

        std::vector<uint8_t> buffer;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteImageCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            throw std::runtime_error("Failed to download image: " + std::string(curl_easy_strerror(res)));
        }

        return buffer;
    }

    std::unique_ptr<Image> Image::LoadFromURL(const std::string &url) {
        try {
            return std::make_unique<Image>(url);
        } catch (const std::exception &e) {
            std::cerr << "Failed to load image: " << e.what() << std::endl;
            return nullptr;
        }
    }

    std::shared_ptr<Image> Image::LoadFromURLShared(const std::string &url) {
        try {
            if (url.contains("discordapp.")) {
                std::cerr << "found an image with a disordapp link, skipping and replacing with dummy image\n";
                return std::make_shared<Image>(std::string("https://1000logos.net/wp-content/uploads/2021/06/Discord-logo.png"));
            }
            return std::make_shared<Image>(url);
        } catch (const std::exception &e) {
            std::cerr << "Failed to load image: " << url << " : " << e.what() << std::endl;
            return nullptr;
        }
    }

    Image::~Image() { Release(); }

    void Image::AllocateMemory(uint64_t size) {
        // Generate OpenGL texture
        glGenTextures(1, &m_Impl->texture_id);
        glBindTexture(GL_TEXTURE_2D, m_Impl->texture_id);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Create empty texture
        GLenum format = Utils::InfinityFormatToOpenGLFormat(m_Format);
        GLenum internalFormat = (m_Format == ImageFormat::RGBA32F) ? GL_RGBA32F : GL_RGBA;
        GLenum type = (m_Format == ImageFormat::RGBA32F) ? GL_FLOAT : GL_UNSIGNED_BYTE;

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, type, nullptr);

        // Store ImGui texture ID
        m_Impl->descriptor_set = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(m_Impl->texture_id));
    }

    void Image::Release() { m_Impl->Release(); }

    void Image::SetData(const void *data) {
        if (!data || !m_Impl->texture_id)
            return;

        glBindTexture(GL_TEXTURE_2D, m_Impl->texture_id);
        GLenum format = Utils::InfinityFormatToOpenGLFormat(m_Format);
        GLenum type = (m_Format == ImageFormat::RGBA32F) ? GL_FLOAT : GL_UNSIGNED_BYTE;

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, format, type, data);
    }

    void Image::Resize(uint32_t width, uint32_t height) {
        if (m_Impl->texture_id && m_Width == width && m_Height == height)
            return;

        m_Width = width;
        m_Height = height;

        Release();
        AllocateMemory(m_Width * m_Height * Utils::BytesPerPixel(m_Format));
    }

    void *Image::Decode(const uint8_t *data, const uint64_t bin_size, uint32_t &outWidth, uint32_t &outHeight) {
        int width, height, channels;
        if (uint8_t *decodedData = stbi_load_from_memory(data, bin_size, &width, &height, &channels, STBI_rgb_alpha)) {
            outWidth = static_cast<uint32_t>(width);
            outHeight = static_cast<uint32_t>(height);
            return decodedData;
        }
        width = 0; // In case stb image modifies these even on failure
        height = 0;
        if (WebPGetInfo(data, bin_size, &width, &height)) {
            outWidth = static_cast<uint32_t>(width);
            outHeight = static_cast<uint32_t>(height);
            if (uint8_t *webpData = WebPDecodeRGBA(data, bin_size, &width, &height)) {
                return webpData;
            }
        }
        std::cerr << "Failed to decode image." << std::endl;
        return nullptr;
    }

    void Image::RenderImage(const std::unique_ptr<Image> &image, const ImVec2 pos, const float scale) {
        if (!image)
            return;
        ImGui::GetWindowDrawList()->AddImage(image->GetDescriptorSet(), pos, {pos.x + image->GetWidth() * scale, pos.y + image->GetHeight() * scale});
    }

    void Image::RenderImage(const std::shared_ptr<Image> &image, const ImVec2 pos, const float scale) {
        if (!image)
            return;
        ImGui::GetWindowDrawList()->AddImage(image->GetDescriptorSet(), pos, {pos.x + image->GetWidth() * scale, pos.y + image->GetHeight() * scale});
    }

    void Image::RenderImage(const std::unique_ptr<Image> &image, const ImVec2 pos, const ImVec2 size) {
        if (!image)
            return;

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

        ImGui::GetWindowDrawList()->AddImage(image->GetDescriptorSet(), pos, {pos.x + size.x, pos.y + size.y}, {0.5f - uv_x / 2.0f, 0.0f}, {0.5f + uv_x / 2.0f, 1.0f});
    }

    void Image::RenderImage(const std::shared_ptr<Image> &image, const ImVec2 pos, const ImVec2 size) {
        if (!image)
            return;

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

        ImGui::GetWindowDrawList()->AddImage(image->GetDescriptorSet(), pos, {pos.x + size.x, pos.y + size.y}, {0.5f - uv_x / 2.0f, 0.0f}, {0.5f + uv_x / 2.0f, 1.0f});
    }

    VkDescriptorSet Image::GetDescriptorSet() const {
        // Return the texture ID cast to VkDescriptorSet for compatibility
        return reinterpret_cast<VkDescriptorSet>(m_Impl->descriptor_set);
    }

    void Image::RenderHomeImage(const std::unique_ptr<Image> &image, const ImVec2 pos, const ImVec2 size, bool is_hovered) {
        if (!image)
            return;

        // Generate a unique ID for this image instance based on its position
        ImGuiID id = ImGui::GetID(std::to_string(pos.x + pos.y).c_str());

        // Initialize animation progress for this instance if it doesn't exist
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

        // Animation parameters
        const float animation_speed = 3.1f;
        const float hover_opacity_boost = 0.3f;
        const float zoom_factor = -0.01f;

        // Update animation progress using the map
        if (is_hovered) {
            s_animation_progress[id] = std::min(1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
        } else {
            s_animation_progress[id] = std::max(0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
        }

        // Number of gradient segments
        const int segments = 60;

        // Calculate segment dimensions
        const float segment_height = size.y / segments;
        const float uv_segment_height = 1.0f / segments;

        // Calculate zoom effect on UV coordinates
        float zoom_amount = zoom_factor * s_animation_progress[id];
        float uv_x = base_uv_x * (1.0f + zoom_amount);

        // UV coordinates for x-axis with centered zoom
        const float uv_left = 0.5f - uv_x / 2.0f;
        const float uv_right = 0.5f + uv_x / 2.0f;

        // Draw each segment with varying alpha
        for (int i = 0; i < segments; i++) {
            float y_start = pos.y + (i * segment_height);
            float y_end = y_start + segment_height;

            // Calculate UV y coordinates with centered zoom
            float base_uv_y_start = i * uv_segment_height;
            float base_uv_y_end = (i + 1) * uv_segment_height;

            // Apply zoom to Y coordinates
            float uv_y_center = 0.5f;
            float uv_y_start = uv_y_center + (base_uv_y_start - uv_y_center) * (1.0f + zoom_amount);
            float uv_y_end = uv_y_center + (base_uv_y_end - uv_y_center) * (1.0f + zoom_amount);

            // Base alpha calculation (transparent to opaque)
            float base_alpha = (float) i / segments;

            // Apply hover effect
            float hover_boost = hover_opacity_boost * s_animation_progress[id];
            float alpha = std::min(1.0f, base_alpha + hover_boost);

            ImVec4 tint_color(1.0f, 1.0f, 1.0f, alpha);

            draw_list->AddImage(image->GetDescriptorSet(), ImVec2(pos.x, y_start), ImVec2(pos.x + size.x, y_end), ImVec2(uv_left, uv_y_start), ImVec2(uv_right, uv_y_end),
                                ImGui::ColorConvertFloat4ToU32(tint_color));
        }
    }

    void Image::RenderHomeImage(const std::shared_ptr<Image> &image, const ImVec2 pos, const ImVec2 size, bool is_hovered) {
        if (!image)
            return;

        // Generate a unique ID for this image instance based on its position
        ImGuiID id = ImGui::GetID(std::to_string(pos.x + pos.y).c_str());

        // Initialize animation progress for this instance if it doesn't exist
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

        // Animation parameters
        const float animation_speed = 3.1f;
        const float hover_opacity_boost = 0.3f;
        const float zoom_factor = -0.01f;

        // Update animation progress using the map
        if (is_hovered) {
            s_animation_progress[id] = std::min(1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
        } else {
            s_animation_progress[id] = std::max(0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
        }

        // Number of gradient segments
        const int segments = 60;

        // Calculate segment dimensions
        const float segment_height = size.y / segments;
        const float uv_segment_height = 1.0f / segments;

        // Calculate zoom effect on UV coordinates
        float zoom_amount = zoom_factor * s_animation_progress[id];
        float uv_x = base_uv_x * (1.0f + zoom_amount);

        // UV coordinates for x-axis with centered zoom
        const float uv_left = 0.5f - uv_x / 2.0f;
        const float uv_right = 0.5f + uv_x / 2.0f;

        // Draw each segment with varying alpha
        for (int i = 0; i < segments; i++) {
            float y_start = pos.y + (i * segment_height);
            float y_end = y_start + segment_height;

            // Calculate UV y coordinates with centered zoom
            float base_uv_y_start = i * uv_segment_height;
            float base_uv_y_end = (i + 1) * uv_segment_height;

            // Apply zoom to Y coordinates
            float uv_y_center = 0.5f;
            float uv_y_start = uv_y_center + (base_uv_y_start - uv_y_center) * (1.0f + zoom_amount);
            float uv_y_end = uv_y_center + (base_uv_y_end - uv_y_center) * (1.0f + zoom_amount);

            // Base alpha calculation (transparent to opaque)
            float base_alpha = (float) i / segments;

            // Apply hover effect
            float hover_boost = hover_opacity_boost * s_animation_progress[id];
            float alpha = std::min(1.0f, base_alpha + hover_boost);

            ImVec4 tint_color(1.0f, 1.0f, 1.0f, alpha);

            draw_list->AddImage(image->GetDescriptorSet(), ImVec2(pos.x, y_start), ImVec2(pos.x + size.x, y_end), ImVec2(uv_left, uv_y_start), ImVec2(uv_right, uv_y_end),
                                ImGui::ColorConvertFloat4ToU32(tint_color));
        }
    }
} // namespace Infinity
