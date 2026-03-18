#include "ResourceManager.hpp"

namespace vkrt {

ResourceManager::ResourceManager(VulkanContext& ctx) 
: _vulkanContext(ctx)
, _instance(ctx.instance)
, _physicalDevice(ctx.physicalDevice)
, _device(ctx.device) {
    // Initialize VMA
    VmaAllocatorCreateInfo allocatorCreateInfo{};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice = _physicalDevice;
    allocatorCreateInfo.device = _device;
    allocatorCreateInfo.instance = _instance;
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VmaVulkanFunctions vulkanFunctions;
    VK_REQUIRE_SUCCESS(vmaImportVulkanFunctionsFromVolk(&allocatorCreateInfo, &vulkanFunctions));
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VK_REQUIRE_SUCCESS(vmaCreateAllocator(&allocatorCreateInfo, &allocator));
}

ResourceManager::~ResourceManager() {
    // Cleanup VMA
    vmaDestroyAllocator(allocator);
}

AllocatedBuffer ResourceManager::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags /* = 0 */, VmaMemoryUsage memoryUsage /* = VMA_MEMORY_USAGE_AUTO */) {
    // Use VMA to select the correct memory type and bind it for me
    AllocatedBuffer buf;
    VkBufferCreateInfo bufferInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;

    VK_REQUIRE_SUCCESS(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buf.buffer, &buf.allocation, &buf.info));
    return buf;
}

AllocatedBuffer ResourceManager::uploadBuffer(void* data, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags = 0, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO) {
    // Allocate a staging buffer
    AllocatedBuffer stagingBuffer = createBuffer(size, 
                                                 usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    void* mappedPtr = stagingBuffer.info.pMappedData;

    // Copy the data over
    memcpy(mappedPtr, data, size);

    // For unified machines, we are done
    VkMemoryPropertyFlags memFlags;
    vmaGetMemoryTypeProperties(allocator, stagingBuffer.info.memoryType, &memFlags);

    bool isUnified = (memFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(isUnified) {
        return stagingBuffer;
    } else {
        // Otherwise, upload to a device local buffer
        AllocatedBuffer deviceBuffer = createBuffer(size,
                                                    usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        _vulkanContext.immediateSubmit([&](VkCommandBuffer cmdBuf) {
            VkBufferCopy bufferRegion{};
            bufferRegion.size = size;
            bufferRegion.srcOffset = 0;
            vkCmdCopyBuffer(cmdBuf, stagingBuffer.buffer, deviceBuffer.buffer, 1, &bufferRegion);
        });

        destroyBuffer(stagingBuffer);
        return deviceBuffer;
    }
}

GPUMeshBuffers ResourceManager::uploadMeshBuffer(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices) {
    GPUMeshBuffers meshBuffers;
    meshBuffers.vertexBuffer = uploadBuffer(indices.data(), indices.size_bytes(),
                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    meshBuffers.indexBuffer = uploadBuffer(vertices.data(), vertices.size_bytes(),
                                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | 
                                           VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    VkBufferDeviceAddressInfo vertexBufferAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = meshBuffers.vertexBuffer.buffer };
    meshBuffers.vertexBufferAddress = vkGetBufferDeviceAddress(_device, &vertexBufferAddressInfo);

    VkBufferDeviceAddressInfo indexBufferAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = meshBuffers.indexBuffer.buffer };
    meshBuffers.indexBufferAddress = vkGetBufferDeviceAddress(_device, &indexBufferAddressInfo);

    return meshBuffers;
}

void ResourceManager::destroyBuffer(AllocatedBuffer buffer) {
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

} // namespace vkrt