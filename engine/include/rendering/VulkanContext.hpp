#ifndef VKRT_VULKANCONTEXT_HPP
#define VKRT_VULKANCONTEXT_HPP

#include "VulkanTypes.hpp"

#include <VkBootstrap.h>

#include "Window.hpp"

namespace vkrt {

/**
 * @class VulkanContext
 * @brief Uses RAII principles to create core Vulkan handles.
 * 
 * Throws runtime errors on failure to initialize.
 */
class VulkanContext {
public:
    VulkanContext(Window* window);
    ~VulkanContext();

    VkInstance instance                     = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface                    = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice         = VK_NULL_HANDLE;
    VkDevice device                         = VK_NULL_HANDLE;

    VkQueue graphicsQueue                   = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex       = 0;
    VkQueue presentQueue                    = VK_NULL_HANDLE;
    uint32_t presentQueueFamilyIndex        = 0;

    VkCommandPool immediateCommandPool      = VK_NULL_HANDLE;
    VkCommandBuffer immediateCommandBuffer  = VK_NULL_HANDLE;
    VkFence immediateFence                  = VK_NULL_HANDLE;

    /**
     * @brief Submits a command buffer to the GPU.
     * 
     * Blocks until completed. Submits to a graphics queue.
     */
    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;

private:
    Window* _window;

    vkb::Instance createInstance();
    void createSurface();
    void createDevice(vkb::Instance& vkbInstance);
    void createImmediateResources();
};

} // namespace vkrt

#endif // VKRT_VULKANCONTEXT_HPP