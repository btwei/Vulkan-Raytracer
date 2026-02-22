#ifndef VKRT_RENDERER_HPP
#define VKRT_RENDERER_HPP

#include <functional>
#include <queue>

#include <volk.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "SceneManager.hpp"
#include "VulkanDescriptors.hpp"
#include "VulkanTypes.hpp"
#include "Window.hpp"

namespace vkrt {

static const int NUM_FRAMES_IN_FLIGHT = 2;

struct DeletionQueue {
private:
    std::queue<std::function<bool()>> _deletionQueue;
public:
    void flushQueue() {
        std::queue<std::function<bool()>> _carryOverDeletionQueue;
        for (; !_deletionQueue.empty(); _deletionQueue.pop()) {
            if(!(_deletionQueue.front())()) _carryOverDeletionQueue.push(_deletionQueue.front());
        }
        _deletionQueue = std::move(_carryOverDeletionQueue);
    }
    void pushFunction(std::function<bool()> function) { _deletionQueue.push(function); }
};

struct FrameData {
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;

    VkFence _renderFence;
    VkSemaphore _acquireToRenderSemaphore;

    DescriptorAllocator _descriptorAllocator;
    VkDescriptorSet _descriptorSet1;

    bool _drawImageShouldResize = false;
    AllocatedImage _drawImage;

    VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
    AllocatedBuffer tlasBuffer;

    DeletionQueue _deletionQueue;
};

struct PushConstants {
    glm::mat4 viewMatrix;
    glm::mat4 inverseViewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 inverseProjectionMatrix;
};

class Renderer {
public:
    Renderer(Window* window);
    ~Renderer();

    void init();
    void update();
    void cleanup();

    uint64_t getFrameNumber() { return _frameCount; }

    AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags = 0, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);
    GPUMeshBuffers uploadMesh(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices);
    void destroyBuffer(AllocatedBuffer buffer);
    void enqueueBufferDestruction(AllocatedBuffer buffer);

    AllocatedImage createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage uploadImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    void destroyImage(AllocatedImage image);
    void enqueueImageDestruction(AllocatedImage image);

    BlasResources createBLAS(GPUMeshBuffers meshBuffers, uint32_t vertexCount, uint32_t indexCount);
    void enqueueBlasDestruction(BlasResources blasResources);

    uint32_t uploadMeshResources();
    void freeMeshResources();
    
    void setTLASBuild(std::vector<BlasInstance>&& instances);
    void setTLASUpdate(std::vector<BlasInstance>&& instances);

    void setViewMatrix(glm::mat4 viewMatrix);
    void setProjectionMatrix(glm::mat4 projectionMatrix);

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
    std::vector<VkSemaphore> _swapchainRenderToPresentSemaphores;
    VkFence* _pLatestPresentFence;
    std::vector<VkFence> _swapchainPresentFences;

    VkCommandPool _immediateCommandPool;
    VkCommandBuffer _immediateCommandBuffer;
    VkFence _immediateFence;

    FrameData _frameData[NUM_FRAMES_IN_FLIGHT];

    VkDescriptorSetLayout _descriptorLayout0;
    VkDescriptorSetLayout _descriptorLayout1;

    VkPipelineLayout _raytracingPipelineLayout;
    VkPipeline _raytracingPipeline;

    AllocatedBuffer _sbtBuffer;
    VkDeviceAddress _sbtAddress;
    VkStridedDeviceAddressRegionKHR _raygenRegion;
    VkStridedDeviceAddressRegionKHR _missRegion;
    VkStridedDeviceAddressRegionKHR _hitRegion;
    VkStridedDeviceAddressRegionKHR _callableRegion;

    PushConstants pcs;

    std::vector<BlasInstance> _tlasInstanceList;
    int _tlasFramesToUpdate = 0;
    bool _tlasUseUpdateInsteadOfRebuild = false;

    VmaAllocator _allocator;

    void initVulkanBootstrap();
    void createSwapchainResources(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void resizeSwapchainResources();
    void initCommandResources();
    void initSyncResources();
    void initVMA();
    void initTLAS();
    void initTransforms();
    void initDescriptorSets();
    void initRaytracingPipeline();
    void initShaderBindingTable();
    void initDrawImages();
    void resizeDrawImage();

    void writeDescriptorUpdates(VkImageView swapchainImageView);
    void raytraceScene(VkCommandBuffer cmdBuf);
    void handleResize();
    void handleTLASUpdate();

    FrameData& getCurrentFrame() { return _frameData[_frameCount % NUM_FRAMES_IN_FLIGHT]; }
    FrameData& getPreviousFrame() { return _frameData[(_frameCount - 1) % NUM_FRAMES_IN_FLIGHT]; }
    FrameData& getNextFrame() { return _frameData[(_frameCount + 1) % NUM_FRAMES_IN_FLIGHT]; }
    void immediateGraphicsQueueSubmitBlocking(std::function<void(VkCommandBuffer cmd)>&& function);
    void immediateGraphicsQueueSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
};

} // namespace vkrt

#endif // VKRT_RENDERER_HPP