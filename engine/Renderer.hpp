#ifndef VKRT_RENDERER_HPP
#define VKRT_RENDERER_HPP

#include <functional>

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

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
    void renderScene();
    void cleanup();

    uint64_t getFrameNumber() { return _frameCount; }

    AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags = 0, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);
    //AllocatedBuffer uploadBuffer(std::span<Vertex>& vertices, std::span<uint32_t> indices);
    void destroyBuffer(AllocatedBuffer buffer);

    AllocatedImage createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    void destroyImage(AllocatedImage image);
    //void buildBLAS();
    
private:
    bool _isInitialized = false;
    bool _shouldResize = false;
    uint64_t _frameCount = 0;

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

    VmaAllocator _allocator;

    void initVulkanBootstrap();
    void createSwapchainResources(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void resizeSwapchainResources();
    void initCommandResources();
    void initSyncResources();
    void initVMA();

    void immediateGraphicsQueueSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
};

} // namespace vkrt

#endif // VKRT_RENDERER_HPP