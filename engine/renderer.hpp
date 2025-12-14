#ifndef VKRT_RENDERER_HPP
#define VKRT_RENDERER_HPP

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#include "Window.hpp"

namespace vkrt {

class Renderer {
public:
    Renderer(Window* window);
    ~Renderer();

    void init();
    void submitFrame();
    void cleanup();
    
private:
    bool _isInitialized = false;

    Window* _window = nullptr;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkSurfaceKHR _surface;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;

    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamilyIndex;
    VkQueue _presentQueue;
    uint32_t _presentQueueFamilyIndex;

    VkExtent2D _swapchainExtent;
    VkFormat _swapchainFormat;
    VkSwapchainKHR _swapchain;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    std::vector<VkSemaphore> _swapchainPresentSemaphores;

    void initVulkanBootstrap();
    void createSwapchainResources(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void resizeSwapchainResources();
};

} // namespace vkrt

#endif // VKRT_RENDERER_HPP