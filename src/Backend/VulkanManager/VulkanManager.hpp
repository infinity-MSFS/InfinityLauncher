#pragma once

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include "backends/imgui_impl_vulkan.h"

#include <functional>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace Infinity {
    class VulkanManager {
    public:
        VulkanManager() = default;

        ~VulkanManager() { CleanupVulkan(); }

        void SetupVulkan(const std::vector<const char *> &extensions);

        void SetupVulkanWindow(ImGui_ImplVulkanH_Window *imgui_window, VkSurfaceKHR imgui_surface, const std::pair<int, int> &window_size);

        void CleanupVulkan() const;

        void CleanupVulkanWindow();

        void FrameRender(ImGui_ImplVulkanH_Window *imgui_window, ImDrawData *draw_data);

        void FramePresent(ImGui_ImplVulkanH_Window *window);

        VkAllocationCallbacks *GetVkAllocator() { return m_VkAllocator; }

        VkInstance GetVkInstance() { return m_VkInstance; }

        VkPhysicalDevice GetVkPhysicalDevice() {
            return m_VkPhysicalDevice;
        }

        VkDevice GetVkDevice() {
            return m_VkDevice;
        }

        VkQueue GetVkQueue() {
            return m_VkQueue;
        }

        VkDebugReportCallbackEXT GetVkDebugReportCallbackEXT() {
            return m_VkDebugReportCallbackEXT;
        }

        VkPipelineCache GetVkPipelineCache() {
            return m_VkPipelineCache;
        }

        VkDescriptorPool GetVkDescriptorPool() {
            return m_VkDescriptorPool;
        }

        ImGui_ImplVulkanH_Window *GetWindowData() {
            return &m_WindowData;
        }

        int GetMinImageCount() {
            return m_MinImageCount;
        }

        uint32_t GetQueueFamily() {
            return m_QueueFamily;
        }

        uint32_t GetCurrentFrameIndex() { return m_CurrentFrameIndex; }

        std::vector<std::vector<VkCommandBuffer> > m_AllocatedCommandBuffers;
        std::vector<std::vector<std::function<void()> > > m_ResourceFreeQueue;

        ImGui_ImplVulkanH_Window m_WindowData;

        bool m_SwapChainRebuild = false;

    private
    :
        void CreateInstance(const std::vector<const char *> &extensions);

        void SelectPhysicalDevice();

        void DetermineQueueFamilies();

        void CreateLogicalDevice();

        void CreateDescriptorPool();

    private
    :
        VkAllocationCallbacks *m_VkAllocator = nullptr;
        VkInstance m_VkInstance = nullptr;
        VkPhysicalDevice m_VkPhysicalDevice = nullptr;
        VkDevice m_VkDevice = nullptr;
        VkQueue m_VkQueue = nullptr;
        VkDebugReportCallbackEXT m_VkDebugReportCallbackEXT = nullptr;
        VkPipelineCache m_VkPipelineCache = nullptr;
        VkDescriptorPool m_VkDescriptorPool = nullptr;

        uint32_t m_QueueFamily = static_cast<uint32_t>(-1);
        uint32_t m_CurrentFrameIndex = 0;


        int m_MinImageCount = 2;
    };

    inline void InfinityCheckVkResult(const VkResult result) {
        if (result == VK_SUCCESS) return;
        fprintf(stderr, "[Vulkan] Error: VkResult = %d\n", result);
        if (result < 0) abort();
    }

    inline void glfw_error_callback(const int error, const char *description) {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }
}
