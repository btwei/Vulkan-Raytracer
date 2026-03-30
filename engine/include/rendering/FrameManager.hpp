#ifndef VKRT_FRAMEMANAGER_HPP
#define VKRT_FRAMEMANAGER_HPP

#include "DeletionQueue.hpp"
#include "DescriptorManager.hpp"
#include "ResourceManager.hpp"
#include "VulkanContext.hpp"
#include "VulkanDescriptors.hpp"
#include "VulkanTypes.hpp"

namespace vkrt {
    
static const int NUM_FRAMES_IN_FLIGHT = 2;

struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;

    VkFence renderFence;
    VkSemaphore acquireToRenderSemaphore;

    Descriptor0 descriptorSet0;

    // On window resize, all draw images should be resized,
    // But you must wait until they are not in use
    bool resized = false;
    VkExtent2D resizeExtent;
    AllocatedImage drawImage;

    TlasResources tlasResources;

    DeletionQueue deletionQueue;
};

/**
 * @class FrameManager
 * @brief Handles per frame data in the Renderer class
 */
class FrameManager {
public:
    FrameManager(VulkanContext& ctx, ResourceManager& resourceManager, DescriptorManager& descriptorManager, VkExtent2D swapchainExtent);
    ~FrameManager();

    uint64_t frameCount = 0;
    FrameData frameData[NUM_FRAMES_IN_FLIGHT];

    void beginFrame();
    void resizeDrawImages(VkExtent2D swapchainExtent);
    void cleanupPerFrame();

    uint64_t getFrameNumber() { return frameCount; }

    FrameData& getCurrentFrame() { return frameData[frameCount % NUM_FRAMES_IN_FLIGHT]; }
    FrameData& getPreviousFrame() { return frameData[(frameCount - 1) % NUM_FRAMES_IN_FLIGHT]; }
    FrameData& getNextFrame() { return frameData[(frameCount + 1) % NUM_FRAMES_IN_FLIGHT]; }

private:
    VulkanContext& _vulkanContext;
    VkDevice _device;
    uint32_t _graphicsQueueFamilyIndex;
    ResourceManager& _resourceManager;
    DescriptorManager& _descriptorManager;

    void initCommandResources();
    void initSyncResources();
    void initDrawImages(VkExtent2D swapchainExtent);
    void initDescriptorSets();
    void initTlasResources();

    void handleDrawImageResize();
};

} // namespace vkrt

#endif // VKRT_FRAMEMANAGER_HPP