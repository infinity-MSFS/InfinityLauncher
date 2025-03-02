
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "curl/curl.h"
#include "imgui.h"
#include "vulkan/vulkan.h"

namespace Infinity {
    enum class ImageFormat { None = 0, RGBA, RGBA32F };

    class Image {
    public:
        explicit Image(const std::string &url);

        explicit Image(std::string_view path);

        explicit Image(const std::vector<uint8_t> &bin);

        Image(uint32_t width, uint32_t height, ImageFormat format, const void *data = nullptr);

        ~Image();

        static std::unique_ptr<Image> LoadFromURL(const std::string &url);

        static std::shared_ptr<Image> LoadFromURLShared(const std::string &url);

        static std::vector<uint8_t> FetchFromURL(const std::string &url);

        static std::shared_ptr<Image> ConstructFromBin(const std::vector<uint8_t> &bin);

        void SetData(const void *data);

        [[nodiscard]] VkDescriptorSet GetDescriptorSet() const;

        void Resize(uint32_t width, uint32_t height);

        [[nodiscard]] uint32_t GetWidth() const { return m_Width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_Height; }

        static void *Decode(const uint8_t *data, uint64_t bin_size, uint32_t &outWidth, uint32_t &outHeight);


        /// <summary>
        /// Renders a pre-constructed image with no controls for width and height, only scale
        /// </summary>
        /// <param name="image">Shared pointer to an Infinity::Image</param>
        /// <param name="pos">X and Y position for the image</param>
        /// <param name="scale">Scale of the image 1.0f - 0.0f</param>
        static void RenderImage(const std::unique_ptr<Image> &image, ImVec2 pos, float scale);

        static void RenderImage(const std::shared_ptr<Image> &image, ImVec2 pos, float scale);

        /// <summary>
        /// Renders a pre-constructed image with a specified width and height. The image will always fill the height requirement (if the image width > specified width, the image will be clipped evenly
        /// left and right)
        /// </summary>
        /// <param name="image">Shared pointer to an Infinity::Image</param>
        /// <param name="pos">X any Y position to render the image</param>
        /// <param name="size">Size of the image (Width, Height)</param>
        static void RenderImage(const std::unique_ptr<Image> &image, ImVec2 pos, ImVec2 size);

        static void RenderImage(const std::shared_ptr<Image> &image, ImVec2 pos, ImVec2 size);

        static void RenderHomeImage(const std::unique_ptr<Image> &image, ImVec2 pos, ImVec2 size, bool is_hovered);

        static void RenderHomeImage(const std::shared_ptr<Image> &image, ImVec2 pos, ImVec2 size, bool is_hovered);

        // TODO: overflow for scaling an image inside of a specified frame (hover action for the cards zooms in)

    private:
        void AllocateMemory(uint64_t size);

        void Release();

        struct Impl;
        std::unique_ptr<Impl> m_Impl;

        uint32_t m_Width = 0, m_Height = 0;
        ImageFormat m_Format = ImageFormat::None;
        std::string m_Filepath;

        static std::map<ImGuiID, float> s_animation_progress;
    };

    namespace Utils {
        struct VulkanDeleter {
            void operator()(VkImage image) const;
            void operator()(VkImageView imageView) const;
            void operator()(VkSampler sampler) const;
            void operator()(VkBuffer buffer) const;
            void operator()(VkDeviceMemory memory) const;
            void operator()(VkCommandPool commandPool) const;
            void operator()(VkDescriptorPool descriptorPool) const;
            void operator()(VkDescriptorSetLayout descriptorSetLayout) const;
            void operator()(VkPipelineLayout pipelineLayout) const;
            void operator()(VkPipeline pipeline) const;
            void operator()(VkRenderPass renderPass) const;
            void operator()(VkFramebuffer framebuffer) const;
            void operator()(VkShaderModule shaderModule) const;
            void operator()(VkSemaphore semaphore) const;
            void operator()(VkFence fence) const;
            void operator()(VkSwapchainKHR swapchain) const;
            void operator()(VkSurfaceKHR surface) const;
        };
    } // namespace Utils
} // namespace Infinity
