#ifndef VKRT_RESOURCEMANAGER_HPP
#define VKRT_RESOURCEMANAGER_HPP

#include <vk_mem_alloc.h>

#include "FrameManager.hpp"
#include "VulkanContext.hpp"
#include "VulkanTypes.hpp"

namespace vkrt {

/**
 * @class ResourceManager
 * @brief Handles GPU resources in the Renderer class
 */
class ResourceManager {
public:
    ResourceManager(VulkanContext& ctx);
    ~ResourceManager();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    VmaAllocator allocator;

    AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags = 0, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);
    AllocatedBuffer uploadBuffer(void* data, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags = 0, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);
    GPUMeshBuffers uploadMeshBuffer(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices);
    void destroyBuffer(AllocatedBuffer buffer);
    
    /*
    AllocatedImage createImage();
    AllocatedImage uploadImage();
    void destroyImage();

    AllocatedSampler createSampler();
    void destroySampler();

    BlasResources buildBlas();
    void destroyBlas();
    void enqueueBlasDestruction();

    void updateBlas();
    void compactBlas();

    TlasResources buildTlas();
    void destroyTlas();

    uint32_t uploadMeshResources();
    void freeMeshResources();
    */

private:
    VulkanContext& _vulkanContext;

    VkInstance _instance;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;

};

} // namespace vkrt

#endif // VKRT_RESOURCEMANAGER_HPP