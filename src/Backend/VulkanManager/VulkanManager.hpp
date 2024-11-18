
#pragma once

#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <functional>


namespace Infinity::Vulkan {


    static VkAllocationCallbacks *g_Allocator = nullptr;
    static VkInstance g_Instance = VK_NULL_HANDLE;
    static VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
    static VkDevice g_Device = VK_NULL_HANDLE;
    static uint32_t g_QueueFamily = static_cast<uint32_t>(-1);
    static VkQueue g_Queue = VK_NULL_HANDLE;
    static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
    static VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
    static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

    static ImGui_ImplVulkanH_Window g_MainWindowData;
    static int g_MinImageCount = 2;
    static bool g_SwapChainRebuild = false;

    static std::vector<std::vector<VkCommandBuffer>> s_AllocatedCommandBuffers;
    static std::vector<std::vector<std::function<void()>>> s_ResourceFreeQueue;

    static uint32_t s_CurrentFrameIndex = 0;

#ifdef IMGUI_VULKAN_DEBUG_REPORT
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode,
                                                       const char *pLayerPrefix, const char *pMessage, void *pUserData) {
        (void) flags;
        (void) object;
        (void) location;
        (void) messageCode;
        (void) pUserData;
        (void) pLayerPrefix;
        return VK_FALSE;
    }
#endif // IMGUI_VULKAN_DEBUG_REPORT

    inline void inf_check_vk_result(const VkResult a_Result) {

        if (a_Result == 0)
            return;
        fprintf(stderr, "[vulkan] Error: VkResult = %d\n", a_Result);
        if (a_Result < 0)
            abort();
    }

    static void SetupVulkan(const char **extensions, uint32_t extensions_count) {
        VkResult err;
        {
            VkInstanceCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            create_info.enabledExtensionCount = extensions_count;
            create_info.ppEnabledExtensionNames = extensions;
#ifdef IMGUI_VULKAN_DEBUG_REPORT
            const char *layers[] = {"VK_LAYER_KHRONOS_validation"};
            create_info.enabledLayerCount = 1;
            create_info.ppEnabledLayerNames = layers;

            const char **extensions_ext = (const char **) malloc(sizeof(const char *) * (extensions_count + 1));
            memcpy(extensions_ext, extensions, extensions_count * sizeof(const char *));
            extensions_ext[extensions_count] = "VK_EXT_debug_report";
            create_info.enabledExtensionCount = extensions_count + 1;
            create_info.ppEnabledExtensionNames = extensions_ext;

            err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
            inf_check_vk_result(err);
            free(extensions_ext);

            auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
            IM_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

            VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
            debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            debug_report_ci.pfnCallback = debug_report;
            debug_report_ci.pUserData = NULL;
            err = vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
            inf_check_vk_result(err);
#else
            err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
            inf_check_vk_result(err);
            IM_UNUSED(g_DebugReport);
#endif
        }
        {
            uint32_t gpu_count;
            err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, nullptr);
            inf_check_vk_result(err);
            IM_ASSERT(gpu_count > 0);

            auto *gpus = static_cast<VkPhysicalDevice *>(malloc(sizeof(VkPhysicalDevice) * gpu_count));
            err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus);
            inf_check_vk_result(err);

            int use_gpu = 0;
            for (int i = 0; i < static_cast<int>(gpu_count); i++) {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(gpus[i], &properties);
                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                    use_gpu = i;
                    break;
                }
            }

            g_PhysicalDevice = gpus[use_gpu];
            free(gpus);
        }
        {
            uint32_t count;
            vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, nullptr);
            auto *queues = static_cast<VkQueueFamilyProperties *>(malloc(sizeof(VkQueueFamilyProperties) * count));
            vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
            for (uint32_t i = 0; i < count; i++)
                if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    g_QueueFamily = i;
                    break;
                }
            free(queues);
            IM_ASSERT(g_QueueFamily != static_cast<uint32_t>(-1));
        }
        {
            constexpr int device_extension_count = 1;
            const char *device_extensions[] = {"VK_KHR_swapchain"};
            constexpr float queue_priority[] = {1.0f};
            VkDeviceQueueCreateInfo queue_info[1] = {};
            queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[0].queueFamilyIndex = g_QueueFamily;
            queue_info[0].queueCount = 1;
            queue_info[0].pQueuePriorities = queue_priority;
            VkDeviceCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            create_info.queueCreateInfoCount = std::size(queue_info);
            create_info.pQueueCreateInfos = queue_info;
            create_info.enabledExtensionCount = device_extension_count;
            create_info.ppEnabledExtensionNames = device_extensions;
            err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
            inf_check_vk_result(err);
            vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
        }
        {
            const VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                                       {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                                       {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                                       {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                                       {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                                       {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                                       {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                                       {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                                       {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
            pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
            pool_info.pPoolSizes = pool_sizes;
            err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
            inf_check_vk_result(err);
        }
    }

    static void SetupVulkanWindow(ImGui_ImplVulkanH_Window *a_Window, VkSurfaceKHR a_Surface, const int width, const int height) {
        a_Window->Surface = a_Surface;

        VkBool32 res;
        vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, a_Window->Surface, &res);
        if (res != VK_TRUE) {
            fprintf(stderr, "Error no WSI support on GPU 0 \n");
            exit(-1);
        }
        constexpr VkFormat requestSurfaceImageFormat[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_A8B8G8R8_UNORM_PACK32, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};

        constexpr VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        a_Window->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, a_Window->Surface, requestSurfaceImageFormat, static_cast<size_t>(IM_ARRAYSIZE(requestSurfaceImageFormat)),
                                                                        requestSurfaceColorSpace);

#ifdef IMGUI_UNLIMITED_FRAME_RATE
        VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
#else
        VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif
        a_Window->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, a_Window->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
        IM_ASSERT(g_MinImageCount >= 2);
        ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, a_Window, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
    }


    static void CleanupVulkan() {
        vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
        auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
        vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // IMGUI_VULKAN_DEBUG_REPORT

        vkDestroyDevice(g_Device, g_Allocator);
        vkDestroyInstance(g_Instance, g_Allocator);
    }

    static void CleanupVulkanWindow() { ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator); }

    static void FrameRender(ImGui_ImplVulkanH_Window *a_Window, ImDrawData *draw_data) {

        VkSemaphore image_acquired_semaphore = a_Window->FrameSemaphores[a_Window->SemaphoreIndex].ImageAcquiredSemaphore;
        VkSemaphore render_complete_semaphore = a_Window->FrameSemaphores[a_Window->SemaphoreIndex].RenderCompleteSemaphore;
        VkResult err = vkAcquireNextImageKHR(g_Device, a_Window->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &a_Window->FrameIndex);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
            g_SwapChainRebuild = true;
            return;
        }
        inf_check_vk_result(err);

        s_CurrentFrameIndex = (s_CurrentFrameIndex + 1) % g_MainWindowData.ImageCount;

        const ImGui_ImplVulkanH_Frame *fd = &a_Window->Frames[a_Window->FrameIndex];
        {
            err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
            // wait indefinitely instead of periodically checking
            inf_check_vk_result(err);

            err = vkResetFences(g_Device, 1, &fd->Fence);
            inf_check_vk_result(err);
        }
        {
            for (auto &func: s_ResourceFreeQueue[s_CurrentFrameIndex])
                func();
            s_ResourceFreeQueue[s_CurrentFrameIndex].clear();
        }
        {
            auto &allocatedCommandBuffers = s_AllocatedCommandBuffers[a_Window->FrameIndex];
            if (!allocatedCommandBuffers.empty()) {
                vkFreeCommandBuffers(g_Device, fd->CommandPool, static_cast<uint32_t>(allocatedCommandBuffers.size()), allocatedCommandBuffers.data());
                allocatedCommandBuffers.clear();
            }

            err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
            inf_check_vk_result(err);
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
            inf_check_vk_result(err);
        }
        {
            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = a_Window->RenderPass;
            info.framebuffer = fd->Framebuffer;
            info.renderArea.extent.width = a_Window->Width;
            info.renderArea.extent.height = a_Window->Height;
            info.clearValueCount = 1;
            info.pClearValues = &a_Window->ClearValue;
            vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        }

        ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

        vkCmdEndRenderPass(fd->CommandBuffer);
        {
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

            err = vkEndCommandBuffer(fd->CommandBuffer);
            inf_check_vk_result(err);
            err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
            inf_check_vk_result(err);
        }
    }

    static void FramePresent(ImGui_ImplVulkanH_Window *wd) {
        if (g_SwapChainRebuild)
            return;
        VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        VkPresentInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &render_complete_semaphore;
        info.swapchainCount = 1;
        info.pSwapchains = &wd->Swapchain;
        info.pImageIndices = &wd->FrameIndex;
        const VkResult err = vkQueuePresentKHR(g_Queue, &info);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
            g_SwapChainRebuild = true;
            return;
        }
        inf_check_vk_result(err);
        wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount;
    }

    static void glfw_error_callback(const int error, const char *description) { fprintf(stderr, "Glfw Error %d: %s\n", error, description); }

} // namespace InfinityRenderer::Vulkan