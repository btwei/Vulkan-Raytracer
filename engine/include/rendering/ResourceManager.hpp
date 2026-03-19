#ifndef VKRT_RESOURCEMANAGER_HPP
#define VKRT_RESOURCEMANAGER_HPP

#include <span>

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
    
    AllocatedImage createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage uploadImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    void destroyImage(AllocatedImage image);

    // Todo: add sampler builder
    void destroySampler(VkSampler sampler);

    struct SubmeshInfo {
        uint32_t triangleCount = 0;
        uint32_t triangleOffset = 0;
    };

    BlasResources buildBlas(GPUMeshBuffers meshBuffers, uint32_t vertexCount, uint32_t indexCount, const std::vector<SubmeshInfo>& submeshRanges);
    void destroyBlas(BlasResources blasResources);

    TlasResources buildTlas(const std::vector<BlasInstance>& blasInstances);
    void destroyTlas(TlasResources tlasResources);

private:
    VulkanContext& _vulkanContext;

    VkInstance _instance;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;

};

} // namespace vkrt

#endif // VKRT_RESOURCEMANAGER_HPP