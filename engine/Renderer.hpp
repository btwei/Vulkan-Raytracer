#ifndef VKRT_RENDERER_HPP
#define VKRT_RENDERER_HPP

#include <functional>

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#include "SceneManager.hpp"
#include "VulkanTypes.hpp"
#include "Window.hpp"

namespace vkrt {

static const int NUM_FRAMES_IN_FLIGHT = 2;

struct FrameData {
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;

    VkFence _renderFence;
    VkFence _swapchainFence;
    VkSemaphore _acquireToRenderSemaphore;
    VkSemaphore _renderToPresentSemaphore;
};

class Renderer {
public:
    Renderer(Window* window);
    ~Renderer();

    void init();
    void renderScene(SceneManager* sceneManager);
    void cleanup();

    //AllocatedBuffer uploadBuffer();
    //AllocatedImage uploadImage();
    //void buildBLAS();
    
private:
    bool _isInitialized = false;
    bool _shouldResize = false;

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

    VkCommandPool _immediateCommandPool;
    VkCommandBuffer _immediateCommandBuffer;
    VkFence _immediateFence;

    FrameData _frameData[NUM_FRAMES_IN_FLIGHT];

    void initVulkanBootstrap();
    void createSwapchainResources(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void resizeSwapchainResources();
    void initCommandResources();
    void initSyncResources();

    void immediateGraphicsQueueSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
};

} // namespace vkrt

#endif // VKRT_RENDERER_HPP