#include "ResourceManager.hpp"

#include "VulkanInitializers.hpp"
#include "VulkanUtils.hpp"

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

    initGlobalResources();
}

ResourceManager::~ResourceManager() {
    // Cleanup global resources
    destroyBuffer(materialInstanceArray);

    for(std::optional<AllocatedImage>& slot : textureArray) {
        if(slot.has_value()) destroyImage(slot.value());
    }

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

AllocatedBuffer ResourceManager::uploadBuffer(void* data, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags /* = 0 */, VmaMemoryUsage memoryUsage /* = VMA_MEMORY_USAGE_AUTO */) {
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

AllocatedImage ResourceManager::createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped /* = false */) {
    // Create the allocated image and set the format and extent immediately
    AllocatedImage image;
    image.imageFormat = format;
    image.imageExtent = extent;

    VkImageCreateInfo imageInfo = init::defaultImageInfo(extent, format, usage);
    if(mipmapped) imageInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Create the image via VMA and attach the image and allocation to the AllocatedImage
    VK_REQUIRE_SUCCESS(vmaCreateImage(allocator, &imageInfo, &allocInfo, &image.image, &image.allocation, nullptr));

    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if(format == VK_FORMAT_D32_SFLOAT) aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;

    VkImageViewCreateInfo viewInfo = init::defaultImageViewInfo(image.image, image.imageFormat, aspectFlag);
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;

    // Finally, create the image view and also attach it to the AllocatedImage
    VK_REQUIRE_SUCCESS(vkCreateImageView(_device, &viewInfo, nullptr, &image.imageView));
    return image;
}

AllocatedImage ResourceManager::uploadImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {
    size_t dataSize = extent.width * extent.height * extent.depth * 4;
    AllocatedBuffer stagingBuffer = createBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    memcpy(stagingBuffer.info.pMappedData, data, dataSize);

    uint32_t mipLevels = 1;
    if(extent.depth != 1) mipmapped = false; // Mipmapping currently doesn't handle 3D textures (and I don't anticipate that I will be using them)
    if(mipmapped) mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
    AllocatedImage newImage = createImage(extent, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    _vulkanContext.immediateSubmit([&](VkCommandBuffer cmdBuf) {
        utils::defaultImageTransition(cmdBuf, newImage.image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                      mipLevels);

        VkBufferImageCopy bufferRegion{};
        bufferRegion.imageExtent = extent;
        bufferRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferRegion.imageSubresource.baseArrayLayer = 0;
        bufferRegion.imageSubresource.layerCount = 1;
        bufferRegion.imageSubresource.mipLevel = 0;

        vkCmdCopyBufferToImage(cmdBuf, stagingBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferRegion);

        if(mipmapped && mipLevels != 1) {
            utils::generateMipmaps(cmdBuf, newImage.image, VkExtent2D{extent.width, extent.height}, mipLevels);
        } else {
            utils::defaultImageTransition(cmdBuf, newImage.image,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                                      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                      1);
        }
    });

    destroyBuffer(stagingBuffer);
    return newImage;
}

void ResourceManager::destroyImage(AllocatedImage image) {
    vkDestroyImageView(_device, image.imageView, nullptr);
    vmaDestroyImage(allocator,image.image, image.allocation);
}

void ResourceManager::destroySampler(VkSampler sampler) {
    vkDestroySampler(_device, sampler, nullptr);
}

BlasResources ResourceManager::buildBlas(GPUMeshBuffers meshBuffers, uint32_t vertexCount, uint32_t indexCount, const std::vector<SubmeshInfo>& submeshRanges) {
    BlasResources blasResources;

    // Create one geometry per range (which covers all verticies and indices)
    std::vector<VkAccelerationStructureGeometryKHR> geometries;
    for(auto& range : submeshRanges) {
        VkAccelerationStructureGeometryKHR blasGeometry{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        blasGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        blasGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // Temporary, until transparency is implemented at a later time
        blasGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        blasGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        blasGeometry.geometry.triangles.vertexData.deviceAddress = meshBuffers.vertexBufferAddress;
        blasGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
        blasGeometry.geometry.triangles.maxVertex = vertexCount - 1;
        blasGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        blasGeometry.geometry.triangles.indexData.deviceAddress = meshBuffers.indexBufferAddress;
        
        geometries.push_back(blasGeometry);
    }

    VkAccelerationStructureBuildGeometryInfoKHR blasBuildGeometryInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    blasBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blasBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    blasBuildGeometryInfo.geometryCount = geometries.size();
    blasBuildGeometryInfo.pGeometries = geometries.data();

    // Query for scratch and blas buffer sizes
    std::vector<uint32_t> maxPrimitiveCounts(submeshRanges.size(), indexCount / 3);
    VkAccelerationStructureBuildSizesInfoKHR blasBuildSizes{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &blasBuildGeometryInfo, maxPrimitiveCounts.data(), &blasBuildSizes);

    // Create the scratch and blas buffers
    blasResources.blasBuffer = createBuffer(blasBuildSizes.accelerationStructureSize,
                                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);
    AllocatedBuffer scratchBuffer = createBuffer(blasBuildSizes.buildScratchSize,
                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

    VkBufferDeviceAddressInfo scratchAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = scratchBuffer.buffer };
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(_device, &scratchAddressInfo); 

    VkAccelerationStructureCreateInfoKHR blasCreateInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    blasCreateInfo.buffer = blasResources.blasBuffer.buffer;
    blasCreateInfo.offset = 0;
    blasCreateInfo.size = blasBuildSizes.accelerationStructureSize;
    blasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    vkCreateAccelerationStructureKHR(_device, &blasCreateInfo, nullptr, &blasResources.blas);

    blasBuildGeometryInfo.scratchData.deviceAddress = scratchAddress;
    blasBuildGeometryInfo.dstAccelerationStructure = blasResources.blas;

    // For each submesh range, populate a build range struct
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRanges;
    for(auto& submeshRange : submeshRanges) {
        VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.primitiveCount = submeshRange.triangleCount;
        buildRangeInfo.primitiveOffset = submeshRange.triangleOffset * sizeof(uint32_t) * 3;
        buildRangeInfo.firstVertex = 0;
        buildRangeInfo.transformOffset = 0;

        buildRanges.push_back(buildRangeInfo);
    }

    const VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos[] = { buildRanges.data() };
    _vulkanContext.immediateSubmit([&](VkCommandBuffer cmdBuf) {
        vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &blasBuildGeometryInfo, buildRangeInfos);
    });

    destroyBuffer(scratchBuffer);

    VkAccelerationStructureDeviceAddressInfoKHR blasAddressInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
    blasAddressInfo.accelerationStructure = blasResources.blas;
    blasResources.blasAddress = vkGetAccelerationStructureDeviceAddressKHR(_device, &blasAddressInfo);

    return blasResources;
}

void ResourceManager::destroyBlas(BlasResources blasResources) {
    vkDestroyAccelerationStructureKHR(_device, blasResources.blas, nullptr);
    destroyBuffer(blasResources.blasBuffer);
}

TlasResources ResourceManager::buildTlas(const std::vector<BlasInstance>& blasInstances) {
    TlasResources tlasResources;

    // Read the instance list into VkAccelerationStructureInstanceKHR structs
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    instances.reserve(blasInstances.size());
    for(const BlasInstance& instance : blasInstances) {
        VkAccelerationStructureInstanceKHR instanceInfo{};
        instanceInfo.transform = instance.transform;
        instanceInfo.instanceCustomIndex = instance.instanceIndex;
        instanceInfo.accelerationStructureReference = instance.blasAddress;
        instanceInfo.instanceShaderBindingTableRecordOffset = 0; // Material models will be selected inside the shader itself
        instanceInfo.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instanceInfo.mask = 0xFF;
        instances.push_back(instanceInfo);
    }
    
    // Create and upload a buffer with the instance data
    AllocatedBuffer tlasInstanceBuffer = createBuffer(instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
                                                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    void* data = tlasInstanceBuffer.info.pMappedData;
    memcpy(data, instances.data(), instances.size() * sizeof(VkAccelerationStructureInstanceKHR));

    // Fetch buffer device address
    VkBufferDeviceAddressInfo tlasInstanceBufferAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    tlasInstanceBufferAddressInfo.buffer = tlasInstanceBuffer.buffer;
    VkDeviceAddress tlasInstanceBufferAddress = vkGetBufferDeviceAddress(_device, &tlasInstanceBufferAddressInfo);

    // Construct the geometry of instances
    VkAccelerationStructureGeometryKHR tlasGeometry{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    tlasGeometry.geometry.instances.data.deviceAddress = tlasInstanceBufferAddress;
    
    VkAccelerationStructureBuildGeometryInfoKHR tlasBuildGeometryInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    tlasBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlasBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlasBuildGeometryInfo.geometryCount = 1;
    tlasBuildGeometryInfo.pGeometries = &tlasGeometry;
    
    // Query for scratch and tlas buffer sizes 
    uint32_t maxPrimitiveCounts[] = { static_cast<uint32_t>(instances.size()) };
    VkAccelerationStructureBuildSizesInfoKHR tlasBuildSizes{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(_device,
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &tlasBuildGeometryInfo,
                                            maxPrimitiveCounts,
                                            &tlasBuildSizes);

    // Create the scratch and tlas buffers
    tlasResources.tlasBuffer = createBuffer(tlasBuildSizes.accelerationStructureSize,
                                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);

    AllocatedBuffer scratchBuffer;
    if(tlasBuildSizes.buildScratchSize > 0) {
        scratchBuffer = createBuffer(tlasBuildSizes.buildScratchSize, 
                                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

        VkBufferDeviceAddressInfo scratchBufferAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = scratchBuffer.buffer };
        VkDeviceAddress scratchBufferAddress = vkGetBufferDeviceAddress(_device, &scratchBufferAddressInfo);

        tlasBuildGeometryInfo.scratchData.deviceAddress = scratchBufferAddress;
    } else {
        // Note: scratch buffer size is allowed to be 0, in which case we cannot make a
        // buffer of size 0..
        tlasBuildGeometryInfo.scratchData.deviceAddress = 0;
    }

    VkAccelerationStructureCreateInfoKHR tlasCreateInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    tlasCreateInfo.buffer = tlasResources.tlasBuffer.buffer;
    tlasCreateInfo.offset = 0;
    tlasCreateInfo.size = tlasBuildSizes.accelerationStructureSize;
    tlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    vkCreateAccelerationStructureKHR(_device, &tlasCreateInfo, nullptr, &tlasResources.tlas);
    
    tlasBuildGeometryInfo.dstAccelerationStructure = tlasResources.tlas;

    VkAccelerationStructureBuildRangeInfoKHR tlasRangeInfo{};
    tlasRangeInfo.primitiveCount = instances.size();
    tlasRangeInfo.primitiveOffset = 0;
    tlasRangeInfo.firstVertex = 0;
    tlasRangeInfo.transformOffset = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos[] = { &tlasRangeInfo };
    // Build the TLAS
    _vulkanContext.immediateSubmit([=](VkCommandBuffer cmdBuf){
        vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &tlasBuildGeometryInfo, buildRangeInfos);
    });

    // Cleanup temporary buffers
    if(tlasBuildSizes.buildScratchSize > 0) destroyBuffer(scratchBuffer);
    destroyBuffer(tlasInstanceBuffer);

    return tlasResources;
}

void ResourceManager::destroyTlas(TlasResources tlasResources) {
    vkDestroyAccelerationStructureKHR(_device, tlasResources.tlas, nullptr);
    destroyBuffer(tlasResources.tlasBuffer);
}

void ResourceManager::initGlobalResources() {
    materialInstanceArray = createBuffer(sizeof(MaterialInstance) * 50, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

} // namespace vkrt