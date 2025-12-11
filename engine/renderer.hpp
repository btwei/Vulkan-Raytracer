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

    void initVulkanBootstrap();
};

} // namespace vkrt

#endif // VKRT_RENDERER_HPP