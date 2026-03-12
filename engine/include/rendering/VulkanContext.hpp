#ifndef VKRT_VULKANCONTEXT_HPP
#define VKRT_VULKANCONTEXT_HPP

#include "VulkanTypes.hpp"
#include "Window.hpp"

namespace vkrt {

class VulkanContext {
public:
    VulkanContext(Window* window);
    ~VulkanContext();

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamilyIndex;
    VkQueue presentQueue;
    uint32_t presentQueueFamilyIndex;

private:
    Window* _window;
};

} // namespace vkrt

#endif // VKRT_VULKANCONTEXT_HPP