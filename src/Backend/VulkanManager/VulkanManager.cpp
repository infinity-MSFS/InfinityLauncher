

#include "VulkanManager.hpp"
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <cassert>
#include <functional>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <utility>

namespace Infinity {
    void VulkanManager::SetupVulkan(const std::vector<const char *> &extensions) {
        CreateInstance(extensions);
        SelectPhysicalDevice();
        DetermineQueueFamilies();
        CreateLogicalDevice();
        CreateDescriptorPool();
    }

    void VulkanManager::CreateInstance(const std::vector<const char *> &extensions) {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();
        const auto error = vkCreateInstance(&create_info, m_VkAllocator, &m_VkInstance);
        InfinityCheckVkResult(error);
    }

    void VulkanManager::SelectPhysicalDevice() {
        uint32_t gpu_count;
        VkResult err = vkEnumeratePhysicalDevices(m_VkInstance, &gpu_count, nullptr);
        InfinityCheckVkResult(err);
        IM_ASSERT(gpu_count > 0);

        auto *gpus = static_cast<VkPhysicalDevice *>(malloc(sizeof(VkPhysicalDevice) * gpu_count));
        err = vkEnumeratePhysicalDevices(m_VkInstance, &gpu_count, gpus);
        InfinityCheckVkResult(err);

        int use_gpu = 0;
        for (int i = 0; i < static_cast<int>(gpu_count); i++) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(gpus[i], &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                use_gpu = i;
                break;
            }
        }

        m_VkPhysicalDevice = gpus[use_gpu];
        free(gpus);
    }

    void VulkanManager::DetermineQueueFamilies() {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(m_VkPhysicalDevice, &count, nullptr);
        auto *queues = static_cast<VkQueueFamilyProperties *>(malloc(sizeof(VkQueueFamilyProperties) * count));
        vkGetPhysicalDeviceQueueFamilyProperties(m_VkPhysicalDevice, &count, queues);
        for (uint32_t i = 0; i < count; i++)
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_QueueFamily = i;
                break;
            }
        free(queues);
        IM_ASSERT(m_QueueFamily != static_cast<uint32_t>(-1));
    }

    void VulkanManager::CreateLogicalDevice() {
        constexpr int device_extension_count = 1;
        const char *device_extensions[] = {"VK_KHR_swapchain"};
        constexpr float queue_priorities[] = {1.0f};
        constexpr float queue_priority[] = {1.0f};
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = m_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = std::size(queue_info);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = device_extension_count;
        create_info.ppEnabledExtensionNames = device_extensions;
        const auto error = vkCreateDevice(m_VkPhysicalDevice, &create_info, m_VkAllocator, &m_VkDevice);
        InfinityCheckVkResult(error);
        vkGetDeviceQueue(m_VkDevice, m_QueueFamily, 0, &m_VkQueue);
    }

    void VulkanManager::CreateDescriptorPool() {
        const VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
        pool_info.pPoolSizes = pool_sizes;
        const auto error = vkCreateDescriptorPool(m_VkDevice, &pool_info, m_VkAllocator, &m_VkDescriptorPool);
        InfinityCheckVkResult(error);
    }

    void VulkanManager::SetupVulkanWindow(ImGui_ImplVulkanH_Window *imgui_window, VkSurfaceKHR imgui_surface, const std::pair<int, int> &window_size) {
        m_WindowData.Surface = imgui_surface;

        VkBool32 result;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_VkPhysicalDevice, m_QueueFamily, m_WindowData.Surface, &result);
        if (result != VK_TRUE) {
            fprintf(stderr, "Error no WSI support on GPU 0\n");
            exit(-1);
        }
        constexpr VkFormat request_surface_image_format[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_A8B8G8R8_UNORM_PACK32, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};

        constexpr VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        m_WindowData.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(m_VkPhysicalDevice, m_WindowData.Surface, request_surface_image_format,
                                                                           static_cast<size_t>(IM_ARRAYSIZE(request_surface_image_format)),
                                                                           requestSurfaceColorSpace);

#ifdef IMGUI_UNLIMITED_FRAME_RATE
        VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
#else
        VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif
        m_WindowData.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(m_VkPhysicalDevice, m_WindowData.Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
        IM_ASSERT(m_MinImageCount >= 2);
        ImGui_ImplVulkanH_CreateOrResizeWindow(m_VkInstance, m_VkPhysicalDevice, m_VkDevice, &m_WindowData, m_QueueFamily, m_VkAllocator, window_size.first, window_size.second, m_MinImageCount);
    }

    void VulkanManager::CleanupVulkan() const {
        vkDestroyDescriptorPool(m_VkDevice, m_VkDescriptorPool, m_VkAllocator);
        vkDestroyDevice(m_VkDevice, m_VkAllocator);
        vkDestroyInstance(m_VkInstance, m_VkAllocator);
    }

    void VulkanManager::CleanupVulkanWindow() {
        ImGui_ImplVulkanH_DestroyWindow(m_VkInstance, m_VkDevice, &m_WindowData, m_VkAllocator);
    }

    void VulkanManager::FramePresent(ImGui_ImplVulkanH_Window *window) {
        if (m_SwapChainRebuild) return;
        VkSemaphore render_semaphore = window->FrameSemaphores[window->SemaphoreIndex].RenderCompleteSemaphore;
        VkPresentInfoKHR info_khr = {};
        info_khr.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info_khr.waitSemaphoreCount = 1;
        info_khr.pWaitSemaphores = &render_semaphore;
        info_khr.swapchainCount = 1;
        info_khr.pSwapchains = &window->Swapchain;
        info_khr.pImageIndices = &window->FrameIndex;
        const auto error = vkQueuePresentKHR(m_VkQueue, &info_khr);
        if (error == VK_ERROR_OUT_OF_DATE_KHR || error == VK_SUBOPTIMAL_KHR) {
            m_SwapChainRebuild = true;
            return;
        }
        InfinityCheckVkResult(error);
        window->SemaphoreIndex = (window->SemaphoreIndex + 1) % window->ImageCount;
    }

    void VulkanManager::FrameRender(ImGui_ImplVulkanH_Window *imgui_window, ImDrawData *draw_data) {
        VkSemaphore image_acquired_semaphore = imgui_window->FrameSemaphores[imgui_window->FrameIndex].ImageAcquiredSemaphore;
        VkSemaphore render_complete_semaphore = imgui_window->FrameSemaphores[imgui_window->SemaphoreIndex].RenderCompleteSemaphore;
        VkResult error = vkAcquireNextImageKHR(m_VkDevice, imgui_window->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &imgui_window->FrameIndex);
        if (error == VK_ERROR_OUT_OF_DATE_KHR || error == VK_SUBOPTIMAL_KHR) {
            m_SwapChainRebuild = true;
            return;
        }
        InfinityCheckVkResult(error);

        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) & m_WindowData.ImageCount;

        const ImGui_ImplVulkanH_Frame *fd = &m_WindowData.Frames[m_WindowData.FrameIndex]; {
            error = vkWaitForFences(m_VkDevice, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
            InfinityCheckVkResult(error);

            error = vkResetFences(m_VkDevice, 1, &fd->Fence);
            InfinityCheckVkResult(error);
        } {
            for (auto &func: m_ResourceFreeQueue[m_CurrentFrameIndex])
                func();
            m_ResourceFreeQueue[m_CurrentFrameIndex].clear();
        } {
            auto &allocatedCommandBuffers = m_AllocatedCommandBuffers[m_WindowData.FrameIndex];
            if (!allocatedCommandBuffers.empty()) {
                vkFreeCommandBuffers(m_VkDevice, fd->CommandPool, static_cast<uint32_t>(allocatedCommandBuffers.size()), allocatedCommandBuffers.data());
                allocatedCommandBuffers.clear();
            }

            error = vkResetCommandPool(m_VkDevice, fd->CommandPool, 0);
            InfinityCheckVkResult(error);
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            error = vkBeginCommandBuffer(fd->CommandBuffer, &info);
            InfinityCheckVkResult(error);
        } {
            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = m_WindowData.RenderPass;
            info.framebuffer = fd->Framebuffer;
            info.renderArea.extent.width = m_WindowData.Width;
            info.renderArea.extent.height = m_WindowData.Height;
            info.clearValueCount = 1;
            info.pClearValues = &m_WindowData.ClearValue;
            vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        }

        ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

        vkCmdEndRenderPass(fd->CommandBuffer); {
            constexpr VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &image_acquired_semaphore;
            info.pWaitDstStageMask = &wait_stage;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &fd->CommandBuffer;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &render_complete_semaphore;

            error = vkEndCommandBuffer(fd->CommandBuffer);
            InfinityCheckVkResult(error);
            error = vkQueueSubmit(m_VkQueue, 1, &info, fd->Fence);
            InfinityCheckVkResult(error);
        }
    }
}


