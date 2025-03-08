
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


        void VulkanDeleter::operator()(VkImage image) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyImage(*device, image, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkImageView imageView) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyImageView(*device, imageView, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkSampler sampler) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroySampler(*device, sampler, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkBuffer buffer) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyBuffer(*device, buffer, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkDeviceMemory memory) const {
            if (const auto device = Application::GetDevice()) {
                vkFreeMemory(*device, memory, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkCommandPool commandPool) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyCommandPool(*device, commandPool, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkDescriptorPool descriptorPool) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyDescriptorPool(*device, descriptorPool, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkDescriptorSetLayout descriptorSetLayout) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyDescriptorSetLayout(*device, descriptorSetLayout, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkPipelineLayout pipelineLayout) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyPipelineLayout(*device, pipelineLayout, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkPipeline pipeline) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyPipeline(*device, pipeline, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkRenderPass renderPass) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyRenderPass(*device, renderPass, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkFramebuffer framebuffer) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyFramebuffer(*device, framebuffer, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkShaderModule shaderModule) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyShaderModule(*device, shaderModule, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkSemaphore semaphore) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroySemaphore(*device, semaphore, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkFence fence) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroyFence(*device, fence, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkSwapchainKHR swapchain) const {
            if (const auto device = Application::GetDevice()) {
                vkDestroySwapchainKHR(*device, swapchain, nullptr);
            }
        }

        void VulkanDeleter::operator()(VkSurfaceKHR surface) const {
            if (const auto instance = Application::GetInstance()) {
                vkDestroySurfaceKHR(*instance, surface, nullptr);
            }
        }

    } // namespace Utils


    class Image::Impl {
    public:
        VkImage image = VK_NULL_HANDLE;
        VkImageView image_view = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkBuffer staging_buffer = VK_NULL_HANDLE;
        VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
        uint64_t aligned_size = 0;

        ~Impl() { Release(); }

        void ReleaseStagingResources() {
            auto release = [](auto &resource, auto deleter) {
                if (auto device = Application::GetDevice().value(); resource) {
                    Application::SubmitResourceFree([res = resource, del = deleter, device] { del(device, res, nullptr); });
                }
            };
            release(staging_buffer, vkDestroyBuffer);
            release(staging_buffer_memory, vkFreeMemory);
        }

        void Release() {
            auto releaseResource = [](auto &resource, auto deleter) {
                if (resource) {
                    Application::SubmitResourceFree([res = resource, del = deleter]() {
                        if (auto device = Application::GetDevice()) {
                            del(res);
                        }
                    });
                    resource = VK_NULL_HANDLE;
                }
            };

            releaseResource(sampler, [](VkSampler s) { vkDestroySampler(*Application::GetDevice(), s, nullptr); });
            releaseResource(image_view, [](VkImageView iv) { vkDestroyImageView(*Application::GetDevice(), iv, nullptr); });
            releaseResource(image, [](VkImage i) { vkDestroyImage(*Application::GetDevice(), i, nullptr); });
            releaseResource(memory, [](VkDeviceMemory m) { vkFreeMemory(*Application::GetDevice(), m, nullptr); });
            releaseResource(staging_buffer, [](VkBuffer b) { vkDestroyBuffer(*Application::GetDevice(), b, nullptr); });
            releaseResource(staging_buffer_memory, [](VkDeviceMemory m) { vkFreeMemory(*Application::GetDevice(), m, nullptr); });
        }
    };


    Image::Image(const uint32_t width, const uint32_t height, const ImageFormat format, const void *data) : m_Impl(std::make_unique<Impl>()), m_Width(width), m_Height(height), m_Format(format) {
        AllocateMemory(m_Width * m_Height * Utils::BytesPerPixel(m_Format));
        if (data) {
            SetData(data);
        }
    }


    static size_t WriteImageCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        auto *buffer = static_cast<std::vector<uint8_t> *>(userp);
        size_t realSize = size * nmemb;
        buffer->insert(buffer->end(), static_cast<uint8_t *>(contents), static_cast<uint8_t *>(contents) + realSize);
        return realSize;
    }

    std::map<ImGuiID, float> Image::s_animation_progress;

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
        if (const auto device_opt = Application::GetDevice(); device_opt.has_value()) {
            VkDevice device = *device_opt;
            VkResult err;

            VkFormat vulkanFormat = Utils::InfinityFormatToVulkanFormat(m_Format);

            VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            image_info.imageType = VK_IMAGE_TYPE_2D;
            image_info.format = vulkanFormat;
            image_info.extent = {m_Width, m_Height, 1};
            image_info.mipLevels = 1;
            image_info.arrayLayers = 1;
            image_info.samples = VK_SAMPLE_COUNT_1_BIT;
            image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
            image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            err = vkCreateImage(device, &image_info, nullptr, &m_Impl->image);
            Vulkan::inf_check_vk_result(err);

            VkMemoryRequirements memory_requirements;
            vkGetImageMemoryRequirements(device, m_Impl->image, &memory_requirements);

            VkMemoryAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            alloc_info.allocationSize = memory_requirements.size;
            alloc_info.memoryTypeIndex = Utils::GetVulkanMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_requirements.memoryTypeBits);

            if (alloc_info.memoryTypeIndex == 0xFFFFFFFF) {
                Errors::Error(Errors::ErrorType::Fatal, "Failed to find suitable memory type for image allocation.").Dispatch();
            }

            err = vkAllocateMemory(device, &alloc_info, nullptr, &m_Impl->memory);
            Vulkan::inf_check_vk_result(err);
            vkBindImageMemory(device, m_Impl->image, m_Impl->memory, 0);

            VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view_info.image = m_Impl->image;
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = vulkanFormat;
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.layerCount = 1;

            err = vkCreateImageView(device, &view_info, nullptr, &m_Impl->image_view);
            Vulkan::inf_check_vk_result(err);

            VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            sampler_info.magFilter = VK_FILTER_LINEAR;
            sampler_info.minFilter = VK_FILTER_LINEAR;
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_info.anisotropyEnable = VK_FALSE;
            sampler_info.maxAnisotropy = 1.0f;
            sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            sampler_info.unnormalizedCoordinates = VK_FALSE;
            sampler_info.compareEnable = VK_FALSE;
            sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            sampler_info.mipLodBias = 0.0f;
            sampler_info.minLod = 0.0f;
            sampler_info.maxLod = 0.0f;

            err = vkCreateSampler(device, &sampler_info, nullptr, &m_Impl->sampler);
            Vulkan::inf_check_vk_result(err);

            m_Impl->descriptor_set = ImGui_ImplVulkan_AddTexture(m_Impl->sampler, m_Impl->image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    void Image::Release() { m_Impl->Release(); }

    void Image::SetData(const void *data) {
        if (const auto device_opt = Application::GetDevice(); device_opt.has_value()) {
            VkDevice device = *device_opt;
            const size_t upload_size = m_Width * m_Height * Utils::BytesPerPixel(m_Format);

            if (!m_Impl->staging_buffer) {
                VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
                bufferInfo.size = upload_size;
                bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                VkResult err = vkCreateBuffer(device, &bufferInfo, nullptr, &m_Impl->staging_buffer);
                Vulkan::inf_check_vk_result(err);

                VkMemoryRequirements memRequirements;
                vkGetBufferMemoryRequirements(device, m_Impl->staging_buffer, &memRequirements);

                VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
                allocInfo.allocationSize = memRequirements.size;
                allocInfo.memoryTypeIndex = Utils::GetVulkanMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memRequirements.memoryTypeBits);

                if (allocInfo.memoryTypeIndex == 0xFFFFFFFF) {
                    throw std::runtime_error("Failed to find staging buffer memory type");
                }

                err = vkAllocateMemory(device, &allocInfo, nullptr, &m_Impl->staging_buffer_memory);
                Vulkan::inf_check_vk_result(err);
                vkBindBufferMemory(device, m_Impl->staging_buffer, m_Impl->staging_buffer_memory, 0);
                m_Impl->aligned_size = memRequirements.size;
            }

            void *mappedData = nullptr;
            vkMapMemory(device, m_Impl->staging_buffer_memory, 0, m_Impl->aligned_size, 0, &mappedData);
            memcpy(mappedData, data, upload_size);
            vkUnmapMemory(device, m_Impl->staging_buffer_memory);

            VkCommandBuffer commandBuffer = Application::GetCommandBuffer(true);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = m_Impl->image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = {m_Width, m_Height, 1};

            vkCmdCopyBufferToImage(commandBuffer, m_Impl->staging_buffer, m_Impl->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            Application::FlushCommandBuffer(commandBuffer);


            m_Impl->ReleaseStagingResources();
        }
    }

    void Image::Resize(uint32_t width, uint32_t height) {
        if (m_Impl->image && m_Width == width && m_Height == height)
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
        ImGui::GetWindowDrawList()->AddImage(image->GetDescriptorSet(), pos, {pos.x + image->GetWidth() * scale, pos.y + image->GetHeight() * scale});
    }

    void Image::RenderImage(const std::shared_ptr<Image> &image, const ImVec2 pos, const float scale) {
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

    VkDescriptorSet Image::GetDescriptorSet() const { return m_Impl->descriptor_set; }


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
#ifdef __linux__
        if (is_hovered) {
            s_animation_progress[id] = std::min(1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
        } else {
            s_animation_progress[id] = std::max(0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
        }
#else
        if (is_hovered) {
            s_animation_progress[id] = min(1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
        } else {
            s_animation_progress[id] = max(0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
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
#ifdef __linux__
        if (is_hovered) {
            s_animation_progress[id] = std::min(1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
        } else {
            s_animation_progress[id] = std::max(0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
        }
#else
        if (is_hovered) {
            s_animation_progress[id] = min(1.0f, s_animation_progress[id] + ImGui::GetIO().DeltaTime * animation_speed);
        } else {
            s_animation_progress[id] = max(0.0f, s_animation_progress[id] - ImGui::GetIO().DeltaTime * animation_speed);
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

            draw_list->AddImage(image->GetDescriptorSet(), ImVec2(pos.x, y_start), ImVec2(pos.x + size.x, y_end), ImVec2(uv_left, uv_y_start), ImVec2(uv_right, uv_y_end),
                                ImGui::ColorConvertFloat4ToU32(tint_color));
        }
    }
} // namespace Infinity
