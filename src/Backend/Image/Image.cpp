
#include "Image.hpp"
#include "Backend/Application/Application.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

#include <algorithm>


#define STB_IMAGE_IMPLEMENTATION
#include <iostream>

#include "Backend/VulkanManager/VulkanManager.hpp"
#include "Util/Error/Error.hpp"
#include "stb_image/stb_image.h"
#include "webp/decode.h"

namespace Infinity {
    namespace Utils {
        static uint32_t GetVulkanMemoryType(const VkMemoryPropertyFlags properties, const uint32_t type_bits) {
            VkPhysicalDeviceMemoryProperties prop;
            if (const auto device = Application::GetPhysicalDevice(); device.has_value()) {
                vkGetPhysicalDeviceMemoryProperties(*device, &prop);
                for (uint32_t i = 0; i < prop.memoryTypeCount; i++) {
                    if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
                        return i;
                }

                return 0xffffffff;
            }
            return 0xffffffff;
        }

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

        static VkFormat InfinityFormatToVulkanFormat(ImageFormat format) {
            switch (format) {
                case ImageFormat::RGBA:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case ImageFormat::RGBA32F:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                case ImageFormat::None:
                    return VK_FORMAT_MAX_ENUM;
            }
            return (VkFormat) 0;
        }
    } // namespace Utils
    Image::Image(const uint32_t width, const uint32_t height, const ImageFormat format,
                 const void *data) : m_Width(width), m_Height(height), m_Format(format) {
        AllocateMemory(m_Width * m_Height * Utils::BytesPerPixel(m_Format));
        if (data)
            SetData(data);
    }

    static size_t WriteImageCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        auto *buffer = static_cast<std::vector<uint8_t> *>(userp);
        size_t realSize = size * nmemb;
        buffer->insert(buffer->end(), static_cast<uint8_t *>(contents), static_cast<uint8_t *>(contents) + realSize);
        return realSize;
    }

    std::map<ImGuiID, float> Image::s_animation_progress;

    Image::Image(const std::string &url) : m_Format(ImageFormat::RGBA) {
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

    Image::Image(const std::vector<uint8_t> &bin): m_Format(ImageFormat::RGBA) {
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
                return std::make_shared<Image>(
                    std::string(
                        "https://1000logos.net/wp-content/uploads/2021/06/Discord-logo.png"));
            }
            return std::make_shared<Image>(url);
        } catch (const std::exception &e) {
            std::cerr << "Failed to load image: " << url << " : " << e.what() << std::endl;
            return nullptr;
        }
    }


    Image::~Image() { Release(); }

    void Image::AllocateMemory(uint64_t size) {
        if (const auto device_opt = Application::GetDevice(); device_opt.has_value()) {
            VkDevice device = *device_opt;

            VkResult err;

            VkFormat vulkanFormat = Utils::InfinityFormatToVulkanFormat(m_Format);

            // Create the Image
            {
                VkImageCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                info.imageType = VK_IMAGE_TYPE_2D;
                info.format = vulkanFormat;
                info.extent.width = m_Width;
                info.extent.height = m_Height;
                info.extent.depth = 1;
                info.mipLevels = 1;
                info.arrayLayers = 1;
                info.samples = VK_SAMPLE_COUNT_1_BIT;
                info.tiling = VK_IMAGE_TILING_OPTIMAL;
                info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                err = vkCreateImage(device, &info, nullptr, &m_Image);
                Vulkan::inf_check_vk_result(err);
                VkMemoryRequirements req;
                vkGetImageMemoryRequirements(device, m_Image, &req);
                VkMemoryAllocateInfo alloc_info = {};
                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                alloc_info.allocationSize = req.size;
                alloc_info.memoryTypeIndex = Utils::GetVulkanMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                                        req.memoryTypeBits);
                err = vkAllocateMemory(device, &alloc_info, nullptr, &m_Memory);
                Vulkan::inf_check_vk_result(err);
                err = vkBindImageMemory(device, m_Image, m_Memory, 0);
                Vulkan::inf_check_vk_result(err);
            }

            // Create the Image View:
            {
                VkImageViewCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.image = m_Image;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.format = vulkanFormat;
                info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                info.subresourceRange.levelCount = 1;
                info.subresourceRange.layerCount = 1;
                err = vkCreateImageView(device, &info, nullptr, &m_ImageView);
                Vulkan::inf_check_vk_result(err);
            }

            // Create sampler:
            {
                VkSamplerCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                info.magFilter = VK_FILTER_LINEAR;
                info.minFilter = VK_FILTER_LINEAR;
                info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                info.minLod = -1000;
                info.maxLod = 1000;
                info.maxAnisotropy = 1.0f;
                err = vkCreateSampler(device, &info, nullptr, &m_Sampler);
                Vulkan::inf_check_vk_result(err);
            }

            // Create the Descriptor Set:
            m_DescriptorSet = (VkDescriptorSet) ImGui_ImplVulkan_AddTexture(
                m_Sampler, m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    void Image::Release() {
        Application::SubmitResourceFree(
            [sampler = m_Sampler, imageView = m_ImageView, image = m_Image, memory = m_Memory, stagingBuffer =
                m_StagingBuffer, stagingBufferMemory = m_StagingBufferMemory]() {
                if (const auto device = Application::GetDevice(); device.has_value()) {
                    vkDestroySampler(*device, sampler, nullptr);
                    vkDestroyImageView(*device, imageView, nullptr);
                    vkDestroyImage(*device, image, nullptr);
                    vkFreeMemory(*device, memory, nullptr);
                    vkDestroyBuffer(*device, stagingBuffer, nullptr);
                    vkFreeMemory(*device, stagingBufferMemory, nullptr);
                };
            });

        m_Sampler = nullptr;
        m_ImageView = nullptr;
        m_Image = nullptr;
        m_Memory = nullptr;
        m_StagingBuffer = nullptr;
        m_StagingBufferMemory = nullptr;
    }

    void Image::SetData(const void *data) {
        if (const auto device_opt = Application::GetDevice(); device_opt.has_value()) {
            VkDevice device = *device_opt;

            size_t upload_size = m_Width * m_Height * Utils::BytesPerPixel(m_Format);

            VkResult err;

            if (!m_StagingBuffer) {
                // Create the Upload Buffer
                {
                    VkBufferCreateInfo buffer_info = {};
                    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    buffer_info.size = upload_size;
                    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    err = vkCreateBuffer(device, &buffer_info, nullptr, &m_StagingBuffer);
                    Vulkan::inf_check_vk_result(err);
                    VkMemoryRequirements req;
                    vkGetBufferMemoryRequirements(device, m_StagingBuffer, &req);
                    m_AlignedSize = req.size;
                    VkMemoryAllocateInfo alloc_info = {};
                    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    alloc_info.allocationSize = req.size;
                    alloc_info.memoryTypeIndex = Utils::GetVulkanMemoryType(
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
                    err = vkAllocateMemory(device, &alloc_info, nullptr, &m_StagingBufferMemory);
                    Vulkan::inf_check_vk_result(err);
                    err = vkBindBufferMemory(device, m_StagingBuffer, m_StagingBufferMemory, 0);
                    Vulkan::inf_check_vk_result(err);
                }
            }

            // Upload to Buffer
            {
                char *map = nullptr;
                err = vkMapMemory(device, m_StagingBufferMemory, 0, m_AlignedSize, 0, (void **) (&map));
                Vulkan::inf_check_vk_result(err);
                memcpy(map, data, upload_size);
                VkMappedMemoryRange range[1] = {};
                range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                range[0].memory = m_StagingBufferMemory;
                range[0].size = m_AlignedSize;
                err = vkFlushMappedMemoryRanges(device, 1, range);
                Vulkan::inf_check_vk_result(err);
                vkUnmapMemory(device, m_StagingBufferMemory);
            }

            // Copy to Image
            {
                VkCommandBuffer command_buffer = Application::GetCommandBuffer(true);

                VkImageMemoryBarrier copy_barrier = {};
                copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                copy_barrier.image = m_Image;
                copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copy_barrier.subresourceRange.levelCount = 1;
                copy_barrier.subresourceRange.layerCount = 1;
                vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                     nullptr, 0, nullptr, 1, &copy_barrier);

                VkBufferImageCopy region = {};
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.layerCount = 1;
                region.imageExtent.width = m_Width;
                region.imageExtent.height = m_Height;
                region.imageExtent.depth = 1;
                vkCmdCopyBufferToImage(command_buffer, m_StagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       1, &region);

                VkImageMemoryBarrier use_barrier = {};
                use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                use_barrier.image = m_Image;
                use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                use_barrier.subresourceRange.levelCount = 1;
                use_barrier.subresourceRange.layerCount = 1;
                vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &use_barrier);

                Application::FlushCommandBuffer(command_buffer);
            }
        }
    }

    void Image::Resize(uint32_t width, uint32_t height) {
        if (m_Image && m_Width == width && m_Height == height)
            return;

        // TODO: max size?

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
        width = 0; // In case stb image fucks with these even if it fails to decode
        height = 0;
        if (WebPGetInfo(data, bin_size, &width, &height)) {
            outWidth = static_cast<uint32_t>(width);
            outHeight = static_cast<uint32_t>(height);
            if (uint8_t *webpData = WebPDecodeRGBA(data, bin_size, &width, &height)) {
                return webpData;
            }
        }
        Errors::Error(Errors::ErrorType::Warning, "Failed to decode image.").Dispatch();
        return nullptr;
    }

    void Image::RenderImage(const std::unique_ptr<Image> &image, const ImVec2 pos, const float scale) {
        ImGui::GetWindowDrawList()->AddImage(image->GetDescriptorSet(), pos, {
                                                 pos.x + image->GetWidth() * scale, pos.y + image->GetHeight() * scale
                                             });
    }

    void Image::RenderImage(const std::shared_ptr<Image> &image, const ImVec2 pos, const float scale) {
        ImGui::GetWindowDrawList()->AddImage(image->GetDescriptorSet(), pos, {
                                                 pos.x + image->GetWidth() * scale, pos.y + image->GetHeight() * scale
                                             });
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

        ImGui::GetWindowDrawList()->AddImage(image->GetDescriptorSet(), pos, {pos.x + size.x, pos.y + size.y},
                                             {0.5f - uv_x / 2.0f, 0.0f}, {0.5f + uv_x / 2.0f, 1.0f});
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

        ImGui::GetWindowDrawList()->AddImage(image->GetDescriptorSet(), pos, {pos.x + size.x, pos.y + size.y},
                                             {0.5f - uv_x / 2.0f, 0.0f}, {0.5f + uv_x / 2.0f, 1.0f});
    }

    void Image::RenderHomeImage(const std::unique_ptr<Image> &image, const ImVec2 pos, const ImVec2 size,
                                bool is_hovered) {
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
#ifdef __linux__
        if (is_hovered) {
            s_animation_progress[id] = std::min(
                1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
        } else {
            s_animation_progress[id] = std::max(
                0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
        }
#else
        if (is_hovered) {
            s_animation_progress[id] = min(
                1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
        } else {
            s_animation_progress[id] = max(
                0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
        }
#endif
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
#ifdef __linux__
            float alpha = std::min(1.0f, base_alpha + hover_boost);
#else
            float alpha = min(1.0f, base_alpha + hover_boost);
#endif

            ImVec4 tint_color(1.0f, 1.0f, 1.0f, alpha);

            draw_list->AddImage(image->GetDescriptorSet(), ImVec2(pos.x, y_start), ImVec2(pos.x + size.x, y_end),
                                ImVec2(uv_left, uv_y_start), ImVec2(uv_right, uv_y_end),
                                ImGui::ColorConvertFloat4ToU32(tint_color));
        }
    }

    void Image::RenderHomeImage(const std::shared_ptr<Image> &image, const ImVec2 pos, const ImVec2 size,
                                bool is_hovered) {
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
#ifdef __linux__
        if (is_hovered) {
            s_animation_progress[id] = std::min(
                1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
        } else {
            s_animation_progress[id] = std::max(
                0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
        }
#else
        if (is_hovered) {
            s_animation_progress[id] = min(
                1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
        } else {
            s_animation_progress[id] = max(
                0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
        }
#endif
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
#ifdef __linux__
            float alpha = std::min(1.0f, base_alpha + hover_boost);
#else
            float alpha = min(1.0f, base_alpha + hover_boost);
#endif

            ImVec4 tint_color(1.0f, 1.0f, 1.0f, alpha);

            draw_list->AddImage(image->GetDescriptorSet(), ImVec2(pos.x, y_start), ImVec2(pos.x + size.x, y_end),
                                ImVec2(uv_left, uv_y_start), ImVec2(uv_right, uv_y_end),
                                ImGui::ColorConvertFloat4ToU32(tint_color));
        }
    }
} // namespace Infinity
