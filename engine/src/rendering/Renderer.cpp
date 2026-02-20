#include "Renderer.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include "VulkanInitializers.hpp"
#include "VulkanUtils.hpp"

namespace vkrt {

Renderer::Renderer(Window* window) : _window(window) {

}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::init() {
    initVulkanBootstrap();
    createSwapchainResources();
    initCommandResources();
    initSyncResources();
    initVMA();
    initTLAS();
    initTransforms();
    initDescriptorSets();
    initRaytracingPipeline();
    initShaderBindingTable();
}

void Renderer::update() {
    handleResize();

    VK_REQUIRE_SUCCESS(vkWaitForFences(_device, 1, &getCurrentFrame()._renderFence, VK_TRUE, 1'000'000'000));

    getCurrentFrame()._deletionQueue.flushQueue();

    // Acquire swapchain image
    uint32_t swapchainImageIndex;
    VkResult imageAcquireResult = vkAcquireNextImageKHR(_device, _swapchain, 1'000'000'000, getCurrentFrame()._acquireToRenderSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);
    if(imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        _shouldResize = true;
        return;
    } else if(imageAcquireResult != VK_SUCCESS && imageAcquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }

    VK_REQUIRE_SUCCESS(vkResetFences(_device, 1, &getCurrentFrame()._renderFence));

    handleTLASUpdate();

    writeDescriptorUpdates(_swapchainImageViews[swapchainImageIndex]);

    // Do rendering
    VkCommandBuffer cmdBuf = getCurrentFrame()._mainCommandBuffer;

    VK_REQUIRE_SUCCESS(vkResetCommandBuffer(cmdBuf, 0));

    VkCommandBufferBeginInfo beginInfo = vkrt::init::defaultCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_REQUIRE_SUCCESS(vkBeginCommandBuffer(cmdBuf, &beginInfo));

        vkrt::utils::defaultImageTransition(cmdBuf, _swapchainImages[swapchainImageIndex],
                                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                            0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                            1);

        VkClearColorValue clearColor{1.0, 0.0, 0.0, 1.0}; 

        VkImageSubresourceRange imageRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        raytraceScene(cmdBuf, _swapchainImages[swapchainImageIndex]);

        vkrt::utils::defaultImageTransition(cmdBuf, _swapchainImages[swapchainImageIndex],
                                            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                            VK_ACCESS_TRANSFER_WRITE_BIT, 0,
                                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                            1);

    VK_REQUIRE_SUCCESS(vkEndCommandBuffer(cmdBuf));

    std::vector<VkCommandBuffer> cmdBufs = {cmdBuf};
    std::vector<VkSemaphore> signalSemaphores = {_swapchainRenderToPresentSemaphores[swapchainImageIndex]};
    std::vector<VkSemaphore> waitSemaphores = {getCurrentFrame()._acquireToRenderSemaphore};
    std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };

    VkSubmitInfo submitInfo = vkrt::init::defaultSubmitInfo(cmdBufs, signalSemaphores, waitSemaphores, waitStages);
    VK_REQUIRE_SUCCESS(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, getCurrentFrame()._renderFence));

    VkPresentInfoKHR presentInfo{ .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_swapchainRenderToPresentSemaphores[swapchainImageIndex];

    presentInfo.pImageIndices = &swapchainImageIndex;

    // Use VkSwapchainPresentFence to track whether the current swapchain image is done being presented
    VkSwapchainPresentFenceInfoKHR presentFenceInfo{ .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR };
    presentInfo.pNext = &presentFenceInfo;
    presentFenceInfo.swapchainCount = 1;

    // Select a free fence; we don't want to block
    // Note that _pLatestPresentFence will always be valid when used during swapchain resizes by updating the pointer immediately after any possible vector resize
    VkFence* pPresentFence = nullptr;
    for(VkFence& fence : _swapchainPresentFences) {
        if(vkGetFenceStatus(_device, fence) == VK_SUCCESS) {
            vkResetFences(_device, 1, &fence);
            pPresentFence = &fence;
            _pLatestPresentFence = &fence;
            break;
        }
    }
    if(pPresentFence == nullptr) {
        VkFence& newFence = _swapchainPresentFences.emplace_back();
        VkFenceCreateInfo fenceInfo = init::defaultFenceInfo();
        vkCreateFence(_device, &fenceInfo, nullptr, &newFence);
        pPresentFence = &newFence;
        _pLatestPresentFence = &newFence;
    }
    presentFenceInfo.pFences = pPresentFence;

    // Submit for presentation
    VkResult presentResult = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
    if(presentResult == VK_ERROR_OUT_OF_DATE_KHR) _shouldResize = true;

    _frameCount++;
}

void Renderer::cleanup() {
    // Generally, destroy objects in reverse order of creation

    if(_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);

        destroyBuffer(_sbtBuffer);

        vkDestroyPipeline(_device, _raytracingPipeline, nullptr);
        vkDestroyPipelineLayout(_device, _raytracingPipelineLayout, nullptr);

        // Cleanup dynamically created present fences
        for(VkFence& fence : _swapchainPresentFences) {
            vkDestroyFence(_device, fence, nullptr);
        }

        // Cleanup frameData
        for(auto& data : _frameData) {
            _frameData->_deletionQueue.flushQueue();

            data._descriptorAllocator.destroy(_device);

            vkDestroyAccelerationStructureKHR(_device, data.tlas, nullptr);
            destroyBuffer(data.tlasBuffer);

            vkDestroyCommandPool(_device, data._commandPool, nullptr);
            vkDestroyFence(_device, data._renderFence, nullptr);
            vkDestroySemaphore(_device, data._acquireToRenderSemaphore, nullptr);
        }

        vkDestroyCommandPool(_device, _immediateCommandPool, nullptr);
        vkDestroyFence(_device, _immediateFence, nullptr);

        vmaDestroyAllocator(_allocator);

        // Cleanup current swapchain resources
        for(int i=0; i < _swapchainImages.size(); i++) {
            vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
            vkDestroySemaphore(_device, _swapchainRenderToPresentSemaphores[i], nullptr);
        }
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        // Cleanup core Vulkan objects
        vkDestroyDevice(_device, nullptr);
    }

    if(_instance != VK_NULL_HANDLE) {
        // Continue cleaning up core Vulkan objects
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debugMessenger, nullptr);
        vkDestroyInstance(_instance, nullptr);
    }
}

AllocatedBuffer Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags /* = 0 */, VmaMemoryUsage memoryUsage /* = VMA_MEMORY_USAGE_AUTO */) {
    AllocatedBuffer buf;
    VkBufferCreateInfo bufferInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;

    VK_REQUIRE_SUCCESS(vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &buf.buffer, &buf.allocation, &buf.info));
    return buf;
}

GPUMeshBuffers Renderer::uploadMesh(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices) {
    GPUMeshBuffers meshBuffers;

    // Create a host visible staging buffer
    AllocatedBuffer stagingBuffer = createBuffer(vertices.size() + indices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    void* data = stagingBuffer.info.pMappedData;

    memcpy(data, vertices.data(), vertices.size_bytes());
    memcpy(static_cast<std::byte*>(data) + vertices.size_bytes(), indices.data(), indices.size_bytes());

    // Create device local buffers and transfer
    meshBuffers.vertexBuffer = createBuffer(vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                                                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | 
                                                             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
                                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    VkBufferDeviceAddressInfo vertexBufferAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = meshBuffers.vertexBuffer.buffer };
    meshBuffers.vertexBufferAddress = vkGetBufferDeviceAddress(_device, &vertexBufferAddressInfo);

    meshBuffers.indexBuffer = createBuffer(indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                           VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    VkBufferDeviceAddressInfo indexBufferAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = meshBuffers.indexBuffer.buffer };
    meshBuffers.indexBufferAddress = vkGetBufferDeviceAddress(_device, &indexBufferAddressInfo);

    immediateGraphicsQueueSubmitBlocking([&](VkCommandBuffer cmdBuf) {
        VkBufferCopy vertexRegion{};
        vertexRegion.size = vertices.size();
        vkCmdCopyBuffer(cmdBuf, stagingBuffer.buffer, meshBuffers.vertexBuffer.buffer, 1, &vertexRegion);

        VkBufferCopy indexRegion{};
        indexRegion.size = indices.size();
        indexRegion.srcOffset = vertices.size();
        vkCmdCopyBuffer(cmdBuf, stagingBuffer.buffer, meshBuffers.indexBuffer.buffer, 1, &indexRegion);
    });

    destroyBuffer(stagingBuffer);

    return meshBuffers;
}

void Renderer::destroyBuffer(AllocatedBuffer buffer) {
    vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
} 

void Renderer::enqueueBufferDestruction(AllocatedBuffer buffer) {
    _frameData[(_frameCount - 1) % NUM_FRAMES_IN_FLIGHT]._deletionQueue.pushFunction([=, this](){
        destroyBuffer(buffer);

        return true;
    });
}

AllocatedImage Renderer::createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {
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
    VK_REQUIRE_SUCCESS(vmaCreateImage(_allocator, &imageInfo, &allocInfo, &image.image, &image.allocation, nullptr));

    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if(format == VK_FORMAT_D32_SFLOAT) aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;

    VkImageViewCreateInfo viewInfo = init::defaultImageViewInfo(image.image, image.imageFormat, aspectFlag);
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;

    // Finally, create the image view and also attach it to the AllocatedImage
    VK_REQUIRE_SUCCESS(vkCreateImageView(_device, &viewInfo, nullptr, &image.imageView));
    return image;
}

AllocatedImage Renderer::uploadImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {
    size_t dataSize = extent.width * extent.height * extent.depth * 4;
    AllocatedBuffer stagingBuffer = createBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    memcpy(stagingBuffer.info.pMappedData, data, dataSize);

    uint32_t mipLevels = 1;
    if(extent.depth != 1) mipmapped = false; // Mipmapping currently doesn't handle 3D textures (and I don't anticipate that I will be using them)
    if(mipmapped) mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
    AllocatedImage newImage = createImage(extent, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    immediateGraphicsQueueSubmitBlocking([&](VkCommandBuffer cmdBuf) {
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

void Renderer::destroyImage(AllocatedImage image) {
    vkDestroyImageView(_device, image.imageView, nullptr);
    vmaDestroyImage(_allocator, image.image, image.allocation);
}

void Renderer::enqueueImageDestruction(AllocatedImage image) {
    _frameData[(_frameCount - 1) % NUM_FRAMES_IN_FLIGHT]._deletionQueue.pushFunction([=, this](){
        destroyImage(image);

        return true;
    });
}

BlasResources Renderer::createBLAS(GPUMeshBuffers meshBuffers, uint32_t vertexCount, uint32_t indexCount) {
    BlasResources blasResources;
    
    VkAccelerationStructureGeometryTrianglesDataKHR triangleData{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
    triangleData.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    triangleData.vertexData.deviceAddress = meshBuffers.vertexBufferAddress;
    triangleData.vertexStride = sizeof(Vertex);
    triangleData.maxVertex = vertexCount - 1;
    triangleData.indexType = VK_INDEX_TYPE_UINT32;
    triangleData.indexData.deviceAddress = meshBuffers.indexBufferAddress;

    VkAccelerationStructureGeometryDataKHR geometryData;
    geometryData.triangles = triangleData;

    VkAccelerationStructureGeometryKHR blasGeometry{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    blasGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    blasGeometry.geometry = geometryData;
    blasGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // Temporary, until transparency is implemented at a later time

    VkAccelerationStructureBuildGeometryInfoKHR blasBuildGeometryInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    blasBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blasBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    blasBuildGeometryInfo.geometryCount = 1;
    blasBuildGeometryInfo.pGeometries = &blasGeometry;

    uint32_t maxPrimitiveCount = 1;
    VkAccelerationStructureBuildSizesInfoKHR blasBuildSizes;
    vkGetAccelerationStructureBuildSizesKHR(_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &blasBuildGeometryInfo, &maxPrimitiveCount, &blasBuildSizes);

    blasResources.blasBuffer = createBuffer(blasBuildSizes.accelerationStructureSize,
                                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);
    AllocatedBuffer scratchBuffer = createBuffer(blasBuildSizes.buildScratchSize, 
                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

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

    VkAccelerationStructureBuildRangeInfoKHR blasRangeInfo{};
    blasRangeInfo.primitiveCount = indexCount / 3;
    blasRangeInfo.primitiveOffset = 0;
    blasRangeInfo.firstVertex = 0;
    blasRangeInfo.transformOffset = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos[] = { &blasRangeInfo };
    immediateGraphicsQueueSubmitBlocking([&](VkCommandBuffer cmdBuf) {
        vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &blasBuildGeometryInfo, buildRangeInfos);
    });

    destroyBuffer(scratchBuffer);

    VkAccelerationStructureDeviceAddressInfoKHR blasAddressInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
    blasAddressInfo.accelerationStructure = blasResources.blas;
    blasResources.blasAddress = vkGetAccelerationStructureDeviceAddressKHR(_device, &blasAddressInfo);

    return blasResources;
}

void Renderer::enqueueBlasDestruction(BlasResources blasResources) {
    _frameData[(_frameCount - 1) % NUM_FRAMES_IN_FLIGHT]._deletionQueue.pushFunction([=, this](){
        vkDestroyAccelerationStructureKHR(_device, blasResources.blas, nullptr);
        destroyBuffer(blasResources.blasBuffer);

        return true;
    });
}

void Renderer::setTLASBuild(std::vector<BlasInstance>&& instances) {
    _tlasInstanceList = instances;
    _tlasFramesToUpdate = NUM_FRAMES_IN_FLIGHT;
    _tlasUseUpdateInsteadOfRebuild = false;
}

void Renderer::setTLASUpdate(std::vector<BlasInstance>&& instances) {
    _tlasInstanceList = instances;
    _tlasFramesToUpdate = NUM_FRAMES_IN_FLIGHT;
    _tlasUseUpdateInsteadOfRebuild = true;
}

void Renderer::setViewMatrix(glm::mat4 viewMatrix) {
    pcs.viewMatrix = viewMatrix;
    pcs.inverseViewMatrix = glm::inverse(viewMatrix);
}

void Renderer::setProjectionMatrix(glm::mat4 projectionMatrix) {
    pcs.projectionMatrix = projectionMatrix;
    pcs.inverseProjectionMatrix = glm::inverse(projectionMatrix);
}

void Renderer::initVulkanBootstrap() {
    // Initialize Volk; Fails if Vulkan loader cannot be located
    VK_REQUIRE_SUCCESS(volkInitialize());

    // For debug builds, enable validation layers
#ifndef NDEBUG
    bool bUseValidationLayers = true;
#else
    bool bUseValidationLayers = false;
#endif

    // Create a Vulkan instance and store it. Throw a runtime error on failure.
    std::vector<const char*> requiredInstanceExtensions = { VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
                                                            VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME}; 

    vkb::InstanceBuilder instance_builder;
    auto instance_ret = instance_builder.set_app_name("VKRT App")
                                        .set_engine_name("VKRT")
                                        .set_engine_version(1, 0, 0)
                                        .require_api_version(1, 3, 0)
                                        .request_validation_layers(bUseValidationLayers)
                                        .enable_extensions(requiredInstanceExtensions)
                                        .use_default_debug_messenger()
                                        .build();

    if(!instance_ret) throw std::runtime_error("Failed to create Vulkan instance. Error: " + instance_ret.error().message());
    _instance = instance_ret.value().instance;
    _debugMessenger = instance_ret.value().debug_messenger;

    // Pass instance to Volk
    volkLoadInstance(_instance);

    // Create a Vulkan Surface via SDL3, which handles platform differences
    _surface = _window->createSurface(_instance);

    // Select a physical device and create a logical device with queues
    std::vector<const char*> requiredExtensions = { VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
                                                    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                                                    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                                    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};

    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features13.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainFeatures{};
    swapchainFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
    swapchainFeatures.swapchainMaintenance1 = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    accelerationFeatures.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    rtPipelineFeatures.rayTracingPipeline = VK_TRUE;

    vkb::PhysicalDeviceSelector deviceSelector{instance_ret.value()};
    auto phys_ret = deviceSelector.set_surface(_surface)
                                  .set_required_features_12(features12)
                                  .set_required_features_13(features13)
                                  .add_required_extensions(requiredExtensions)
                                  .add_required_extension_features(swapchainFeatures)
                                  .add_required_extension_features(accelerationFeatures)
                                  .add_required_extension_features(rtPipelineFeatures)
                                  .set_minimum_version(1, 3)
                                  .select();

    if(!phys_ret) throw std::runtime_error("Failed to find compatible Vulkan physical device with required extensions!");
    _physicalDevice = phys_ret.value().physical_device;

    vkb::DeviceBuilder deviceBuilder{phys_ret.value()};
    auto device_ret = deviceBuilder.build();
    if(!device_ret) throw std::runtime_error("Failed to create Vulkan device!");
    _device = device_ret.value().device;

    // Load device for Volk to reduce dispatch overhead
    volkLoadDevice(_device);

    _graphicsQueue = device_ret.value().get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamilyIndex = device_ret.value().get_queue_index(vkb::QueueType::graphics).value();
    _presentQueue = device_ret.value().get_queue(vkb::QueueType::present).value();
    _presentQueueFamilyIndex = device_ret.value().get_queue_index(vkb::QueueType::present).value();
}

void Renderer::createSwapchainResources(VkSwapchainKHR oldSwapchain /* = VK_NULL_HANDLE */) {
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &surfaceCaps);

    // Select the actual swapchain extent. 
    {
        if(surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            _swapchainExtent = surfaceCaps.currentExtent;
        } else {
            _swapchainExtent = VkExtent2D{std::clamp<uint32_t>(_window->getFramebufferWidth(), surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width),
                                          std::clamp<uint32_t>(_window->getFramebufferHeight(), surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height)};
        }
    }

    // Select the best available swapchain format
    VkColorSpaceKHR swapchainColorSpace;
    {
        uint32_t surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &surfaceFormatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &surfaceFormatCount, surfaceFormats.data());

        _swapchainFormat = surfaceFormats[0].format;
        swapchainColorSpace = surfaceFormats[0].colorSpace;
        for(const auto& format : surfaceFormats) {
            if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                _swapchainFormat = format.format;
                swapchainColorSpace = format.colorSpace;
                break;
            }
        } 
    }

    // Select the best available presentation mode
    VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR; // Always available
    {
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, presentModes.data());

        for(const auto& presentMode : presentModes) {
            if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                selectedPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            }
        }
    }

    // Set image count to the minimum we want, then cap it to the max allowed
    uint32_t imageCount = std::max( 3u, surfaceCaps.minImageCount);
    imageCount = ( surfaceCaps.maxImageCount > 0 && imageCount > surfaceCaps.maxImageCount) ? surfaceCaps.maxImageCount : imageCount;

    VkSwapchainCreateInfoKHR swapchainInfo{ .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchainInfo.oldSwapchain = oldSwapchain;
    swapchainInfo.surface = _surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = _swapchainFormat;
    swapchainInfo.imageColorSpace = swapchainColorSpace;
    swapchainInfo.imageExtent = _swapchainExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchainInfo.preTransform = surfaceCaps.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = selectedPresentMode;
    swapchainInfo.clipped = true;

    uint32_t queueFamilyIndices[] = {_graphicsQueueFamilyIndex, _presentQueueFamilyIndex};
    if(_graphicsQueueFamilyIndex != _presentQueueFamilyIndex) {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    if(vkCreateSwapchainKHR(_device, &swapchainInfo, nullptr, &_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan swapchain!");
    }

    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(_device, _swapchain, &swapchainImageCount, nullptr);
    _swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(_device, _swapchain, &swapchainImageCount, _swapchainImages.data());

    _swapchainImageViews.clear();
    for(int i=0; i<swapchainImageCount; i++) {
        VkImageViewCreateInfo imageViewInfo = init::defaultImageViewInfo(_swapchainImages[i], _swapchainFormat);
        VkImageView imageView;
        if(vkCreateImageView(_device, &imageViewInfo, nullptr, &imageView) != VK_SUCCESS) throw std::runtime_error("Failed to create Vulkan swapchain image view!");
        _swapchainImageViews.push_back(imageView);
    }

    // Create semaphores to guard the swapchain images
    // - the vector containing them must be the same size as the swapchain image count
    _swapchainRenderToPresentSemaphores.resize(_swapchainImages.size());
    for(int i = 0; i < _swapchainImages.size(); i++) {
        VkSemaphoreCreateInfo semaphoreInfo = init::defaultSemaphoreInfo();
        VK_REQUIRE_SUCCESS(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_swapchainRenderToPresentSemaphores[i]));
    }
}

void Renderer::resizeSwapchainResources() {
    // The key to smooth resizing is to not call vkWaitDevice and instead push these to be destroyed later
    // If not the first frame, push resources into a destruction queue
    if(_pLatestPresentFence != nullptr) {
        getCurrentFrame()._deletionQueue.pushFunction([=,
                                                       _device = _device,
                                                       latestPresentFence = *_pLatestPresentFence,
                                                       imageViews = _swapchainImageViews,
                                                       semaphores = _swapchainRenderToPresentSemaphores,
                                                       _swapchain = _swapchain]() {
                                                    
            // Check the present fence associated with the swapchain resource
            if(vkGetFenceStatus(_device, latestPresentFence) == VK_SUCCESS) {
                // If true, cleanup all associated resources
                for(int i = 0; i<imageViews.size(); i++) {
                    vkDestroyImageView(_device, imageViews[i], nullptr);
                    vkDestroySemaphore(_device, semaphores[i], nullptr);
                }
                vkDestroySwapchainKHR(_device, _swapchain, nullptr);

                vkDestroyFence(_device, latestPresentFence, nullptr);
                return true;
            }
            // Otherwise, return false
            return false;
        });

        // Then replace the present fence that we took out
        VkFenceCreateInfo fenceInfo = vkrt::init::defaultFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        vkCreateFence(_device, &fenceInfo, nullptr, _pLatestPresentFence);

    } else {
        // A recreation was requested before the first frame; Destroy resources immediately
        for(int i = 0; i<_swapchainImageViews.size(); i++) {
            vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
            vkDestroySemaphore(_device, _swapchainRenderToPresentSemaphores[i], nullptr);
        }
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        vkDestroyFence(_device, *_pLatestPresentFence, nullptr);
    }

    // And create new swapchain resources -- that straight up overwrite the old values
    createSwapchainResources(_swapchain);
}

// Initialize command pools and persistant command buffers that are used for immediate commands and per frame in flight
void Renderer::initCommandResources() {
    // Initialize per frame command resources
    for(auto& data : _frameData) {
        VkCommandPoolCreateInfo poolInfo = init::defaultCommandPoolInfo(_graphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                                                                   VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        if(vkCreateCommandPool(_device, &poolInfo, nullptr, &data._commandPool) != VK_SUCCESS) throw std::runtime_error("Failed to create Vulkan command pool!");
        
        VkCommandBufferAllocateInfo allocInfo = init::defaultCommandBufferAllocateInfo(data._commandPool);
        vkAllocateCommandBuffers(_device, &allocInfo, &data._mainCommandBuffer);
    }

    // Initialize immediate command resources
    VkCommandPoolCreateInfo poolInfo = init::defaultCommandPoolInfo(_graphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                                                               VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    if(vkCreateCommandPool(_device, &poolInfo, nullptr, &_immediateCommandPool) != VK_SUCCESS) throw std::runtime_error("Failed to create Vulkan command pool!");
    
    VkCommandBufferAllocateInfo allocInfo = init::defaultCommandBufferAllocateInfo(_immediateCommandPool);
    vkAllocateCommandBuffers(_device, &allocInfo, &_immediateCommandBuffer);
}

void Renderer::initSyncResources() {
    VkFenceCreateInfo fenceInfo = init::defaultFenceInfo();
    VK_REQUIRE_SUCCESS(vkCreateFence(_device, &fenceInfo, nullptr, &_immediateFence));

    for(auto& data : _frameData) {
        VkSemaphoreCreateInfo semaphoreInfo = init::defaultSemaphoreInfo();
        VK_REQUIRE_SUCCESS(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &data._acquireToRenderSemaphore));

        VkFenceCreateInfo fenceInfo = init::defaultFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VK_REQUIRE_SUCCESS(vkCreateFence(_device, &fenceInfo, nullptr, &data._renderFence));
    }
}

void Renderer::initVMA() {
    VmaAllocatorCreateInfo allocatorCreateInfo{};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice = _physicalDevice;
    allocatorCreateInfo.device = _device;
    allocatorCreateInfo.instance = _instance;
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VmaVulkanFunctions vulkanFunctions;
    VK_REQUIRE_SUCCESS(vmaImportVulkanFunctionsFromVolk(&allocatorCreateInfo, &vulkanFunctions));
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VK_REQUIRE_SUCCESS(vmaCreateAllocator(&allocatorCreateInfo, &_allocator));
}

void Renderer::initTLAS() {
    // Set the TLAS to build by default an empty TLAS on the first frame
    // If setTLASBuild is called again before running, it instead will be displayed on the first frame
    setTLASBuild({});
}

void Renderer::initTransforms() {
    // Set default transforms here
    glm::mat4 viewMatrix = glm::mat4(1);
    setViewMatrix(viewMatrix);

    glm::mat4 projectionMatrix = glm::perspective(glm::radians(70.f), (float)_window->getFramebufferWidth() / (float)_window->getFramebufferHeight(), 10000.f, 0.1f);
    setProjectionMatrix(projectionMatrix);
}

void Renderer::initDescriptorSets() {
    // Build our descriptor1 layout and write it to the renderer class for use with pipeline creation
    // My intent is for descriptor0 to be written once and descriptor1 to be written per frame at the moment
    DescriptorLayoutBuilder layout;
    layout.add_binding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    layout.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);

    VK_REQUIRE_SUCCESS(layout.build(_device, &_descriptorLayout1));

    std::vector<VkDescriptorPoolSize> sizes;
    sizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1});
    sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1});

    for(FrameData& data : _frameData) {
        data._descriptorAllocator.init(_device, 1, sizes);

        VK_REQUIRE_SUCCESS(data._descriptorAllocator.allocate(_device, &_descriptorLayout1, &data._descriptorSet1));
    }
}

void Renderer::initRaytracingPipeline() {
    // Load the ray-tracing.spv into a shader module
    std::filesystem::path raytracingSpvPath = std::filesystem::path(_window->getBinaryPath()).parent_path() / "shaders" / "ray-tracing.spv";
    VkShaderModule raytracingShaderModule;
    if(!utils::loadShaderModule(raytracingSpvPath, _device, &raytracingShaderModule)) throw std::runtime_error("Failed to create raytracing shader module!");

    // Create pipeline stages with their entry points
    std::array<VkPipelineShaderStageCreateInfo, 3> stages{};
    stages[0] = init::defaultShaderStageInfo(VK_SHADER_STAGE_RAYGEN_BIT_KHR, raytracingShaderModule, "rayGenerationProgram");
    stages[1] = init::defaultShaderStageInfo(VK_SHADER_STAGE_MISS_BIT_KHR, raytracingShaderModule, "missProgram");
    stages[2] = init::defaultShaderStageInfo(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, raytracingShaderModule, "closestHitProgram");

    // Create shader groups
    std::array<VkRayTracingShaderGroupCreateInfoKHR, 3> groups{};
    groups[0] = init::emptyShaderGroupInfo();
    groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[0].generalShader = 0;

    groups[1] = init::emptyShaderGroupInfo();
    groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[1].generalShader = 1;

    groups[2] = init::emptyShaderGroupInfo();
    groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    groups[2].closestHitShader = 2;

    // Create pipeline layout
    VkPushConstantRange pcRange{};
    pcRange.size = sizeof(PushConstants);
    pcRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR; // subject to change as neccessary

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pcRange;

    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorLayout1;

    VK_REQUIRE_SUCCESS(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_raytracingPipelineLayout));

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{ .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
    pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    pipelineInfo.pGroups = groups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = 3;
    pipelineInfo.layout = _raytracingPipelineLayout;

    VK_REQUIRE_SUCCESS(vkCreateRayTracingPipelinesKHR(_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_raytracingPipeline));

    vkDestroyShaderModule(_device, raytracingShaderModule, nullptr);
}

void Renderer::initShaderBindingTable() {
    // Get raytracing device properties to get info on SBT handle size and alignment
    VkPhysicalDeviceProperties2 props{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
    props.pNext = &rtProps;
    vkGetPhysicalDeviceProperties2(_physicalDevice, &props);
    uint32_t handleSize = rtProps.shaderGroupHandleSize;
    
    // Query rtPipeline for shader group handles
    uint32_t groupCount = 3;
    uint32_t sbtSize = groupCount * handleSize; // groups * bytes / group = # bytes
    std::vector<std::byte> handles(sbtSize); 
    VK_REQUIRE_SUCCESS(vkGetRayTracingShaderGroupHandlesKHR(_device, _raytracingPipeline, 0, groupCount, sbtSize, handles.data()));

    // Compute total buffer size, while respecting alignment
    uint32_t handleAlignment = rtProps.shaderGroupHandleAlignment;
    uint32_t baseAlignment = rtProps.shaderGroupBaseAlignment;

    auto align = [](uint32_t x, uint32_t a) { return (x + a - 1) & ~(a - 1); };

    uint32_t raygenSize = align(handleSize, handleAlignment);
    uint32_t missSize = align(handleSize, handleAlignment);
    uint32_t hitSize = align(handleSize, handleAlignment);

    uint32_t raygenOffset = 0;
    uint32_t missOffset = align(raygenOffset + raygenSize, baseAlignment);
    uint32_t hitOffset = align(missOffset + missSize, baseAlignment);

    size_t bufferSize = hitOffset + hitSize;

    // Create the SBT buffer
    AllocatedBuffer stagingBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    std::byte* mappedDataPtr = reinterpret_cast<std::byte*>(stagingBuffer.info.pMappedData);

        // Copy our one raygen section
        memcpy(mappedDataPtr + raygenOffset, handles.data(), handleSize);

        // Copy our one miss section
        memcpy(mappedDataPtr + missOffset, handles.data() + handleSize, handleSize);

        // Copy our one hit section
        memcpy(mappedDataPtr + hitOffset, handles.data() + 2 * handleSize, handleSize);

    _sbtBuffer = createBuffer(bufferSize,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                              VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

    immediateGraphicsQueueSubmitBlocking([&](VkCommandBuffer cmdBuf){
        VkBufferCopy sbtRegion{};
        sbtRegion.size = bufferSize;
        sbtRegion.srcOffset = 0;
        vkCmdCopyBuffer(cmdBuf, stagingBuffer.buffer, _sbtBuffer.buffer, 1, &sbtRegion);
    });

    VkBufferDeviceAddressInfo addressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    addressInfo.buffer = _sbtBuffer.buffer;
    _sbtAddress = vkGetBufferDeviceAddress(_device, &addressInfo);

    // Populate regions
    _raygenRegion.deviceAddress = _sbtAddress;
    _raygenRegion.stride = raygenSize;
    _raygenRegion.size = raygenSize;

    _missRegion.deviceAddress = _sbtAddress + missOffset;
    _missRegion.stride = missSize;
    _missRegion.size = missSize;

    _hitRegion.deviceAddress = _sbtAddress + hitOffset;
    _hitRegion.stride = hitSize;
    _hitRegion.size = hitSize;

    _callableRegion = {};
}

void Renderer::writeDescriptorUpdates(VkImageView swapchainImageView) {
    // Update per frame descriptors
    // Practically speaking.. these must be updated because frames in flight != swapchain count necessarily
    // But TLAS aren't built every frame necessarily.. perhaps I'll do partial descriptor updates later
    VkWriteDescriptorSetAccelerationStructureKHR tlasInfo{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
    tlasInfo.accelerationStructureCount = 1;
    tlasInfo.pAccelerationStructures = &getCurrentFrame().tlas;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = swapchainImageView;
    imageInfo.sampler = VK_NULL_HANDLE;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = getCurrentFrame()._descriptorSet1;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pNext = &tlasInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = getCurrentFrame()._descriptorSet1;
    descriptorWrites[1].dstBinding = 0;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::raytraceScene(VkCommandBuffer cmdBuf, VkImage image) {
    // Bind the pipeline
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracingPipeline);

    // Bind the descriptor sets
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracingPipelineLayout, 1, 1, &getCurrentFrame()._descriptorSet1, 0, nullptr);

    // Update and Push constants
    // no updates here yet.. matricies may be updated externally via setViewMatrix or setProjectionMatrix
    vkCmdPushConstants(cmdBuf, _raytracingPipelineLayout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(PushConstants), &pcs);

    // Ray trace
    vkCmdTraceRaysKHR(cmdBuf, &_raygenRegion, &_missRegion, &_hitRegion, &_callableRegion, _swapchainExtent.width, _swapchainExtent.height, 1);
}

void Renderer::handleResize() {
    // Get resize request from OS
    if(_window->getWasResized()) _shouldResize = true;

    // Process resize if necessary
    if(_shouldResize) {
        resizeSwapchainResources();
        _shouldResize = false;
    }
}

void Renderer::handleTLASUpdate() {
    // Update the TLAS resources attached to this frame in flight
    if(_tlasFramesToUpdate > 0) {
        // Destroy old resources
        if(getCurrentFrame().tlas != VK_NULL_HANDLE) {
            vkDestroyAccelerationStructureKHR(_device, getCurrentFrame().tlas, nullptr);
            destroyBuffer(getCurrentFrame().tlasBuffer);
        }

        // Buffers cannot be backed by 0 bytes of memory, so we must handle the empty TLAS case separately
        if(_tlasInstanceList.size() > 0) {
            // Read the instance list into VkAccelerationStructureInstanceKHR structs
            std::vector<VkAccelerationStructureInstanceKHR> tlasInstances;
            tlasInstances.reserve(_tlasInstanceList.size());

            for(const BlasInstance& instance : _tlasInstanceList) {
                VkAccelerationStructureInstanceKHR asInstance{};
                asInstance.transform = instance.transform;
                asInstance.instanceCustomIndex = instance.instanceIndex;
                asInstance.accelerationStructureReference = instance.blasAddress;
                asInstance.instanceShaderBindingTableRecordOffset = 0; // May be changed later to support 2-3 material models
                asInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
                asInstance.mask = 0xFF;

                tlasInstances.push_back(asInstance);
            }

            // Create and upload a buffer with the instance data
            AllocatedBuffer tlasInstanceBuffer = createBuffer(tlasInstances.size(),
                                                         VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
            void* data = tlasInstanceBuffer.info.pMappedData;
            memcpy(data, tlasInstances.data(), tlasInstances.size() * sizeof(VkAccelerationStructureInstanceKHR));

            // Fetch buffer device address
            VkBufferDeviceAddressInfo tlasInstanceBufferAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
            tlasInstanceBufferAddressInfo.buffer = tlasInstanceBuffer.buffer;
            VkDeviceAddress tlasInstanceBufferAddress = vkGetBufferDeviceAddress(_device, &tlasInstanceBufferAddressInfo);

            VkAccelerationStructureGeometryInstancesDataKHR instanceData{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
            instanceData.arrayOfPointers = VK_FALSE;
            instanceData.data.deviceAddress = tlasInstanceBufferAddress;

            VkAccelerationStructureGeometryDataKHR geometryData;
            geometryData.instances = instanceData;

            VkAccelerationStructureGeometryKHR tlasGeometry{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
            tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            tlasGeometry.geometry = geometryData;

            VkAccelerationStructureBuildGeometryInfoKHR tlasBuildGeometryInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            tlasBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            tlasBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            tlasBuildGeometryInfo.geometryCount = 1;
            tlasBuildGeometryInfo.pGeometries = &tlasGeometry;

            uint32_t tlasMaxPrimitiveCount[] = { static_cast<uint32_t>(tlasInstances.size()) };

            VkAccelerationStructureBuildSizesInfoKHR tlasBuildSizes;
            vkGetAccelerationStructureBuildSizesKHR(_device,
                                                    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                    &tlasBuildGeometryInfo,
                                                    tlasMaxPrimitiveCount,
                                                    &tlasBuildSizes);

            getCurrentFrame().tlasBuffer = createBuffer(tlasBuildSizes.accelerationStructureSize,
                                                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);
            AllocatedBuffer scratchBuffer = createBuffer(tlasBuildSizes.buildScratchSize, 
                                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

            VkBufferDeviceAddressInfo scratchBufferAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
            scratchBufferAddressInfo.buffer = scratchBuffer.buffer;
            VkDeviceAddress scratchBufferAddress = vkGetBufferDeviceAddress(_device, &scratchBufferAddressInfo);

            VkAccelerationStructureCreateInfoKHR tlasCreateInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
            tlasCreateInfo.buffer = getCurrentFrame().tlasBuffer.buffer;
            tlasCreateInfo.offset = 0;
            tlasCreateInfo.size = tlasBuildSizes.accelerationStructureSize;
            tlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

            vkCreateAccelerationStructureKHR(_device, &tlasCreateInfo, nullptr, &getCurrentFrame().tlas);
            tlasBuildGeometryInfo.dstAccelerationStructure = getCurrentFrame().tlas;
            tlasBuildGeometryInfo.scratchData.deviceAddress = scratchBufferAddress;

            VkAccelerationStructureBuildRangeInfoKHR tlasRangeInfo{};
            tlasRangeInfo.primitiveCount = tlasInstances.size();
            tlasRangeInfo.primitiveOffset = 0;
            tlasRangeInfo.firstVertex = 0;
            tlasRangeInfo.transformOffset = 0;

            VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos[] = { &tlasRangeInfo };

            // Build the TLAS
            immediateGraphicsQueueSubmitBlocking([=](VkCommandBuffer cmdBuf){
                vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &tlasBuildGeometryInfo, buildRangeInfos);
            });

            destroyBuffer(scratchBuffer);
            destroyBuffer(tlasInstanceBuffer);
        } else {
            // the list of instances is 0 so, we handle this case separately
            VkAccelerationStructureGeometryInstancesDataKHR instanceData{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
            instanceData.arrayOfPointers = VK_FALSE;
            instanceData.data.deviceAddress = 0;

            VkAccelerationStructureGeometryDataKHR geometryData;
            geometryData.instances = instanceData;

            VkAccelerationStructureGeometryKHR tlasGeometry{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
            tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            tlasGeometry.geometry = geometryData;

            VkAccelerationStructureBuildGeometryInfoKHR tlasBuildGeometryInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            tlasBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            tlasBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            tlasBuildGeometryInfo.geometryCount = 1;
            tlasBuildGeometryInfo.pGeometries = &tlasGeometry;

            uint32_t tlasMaxPrimitiveCount[] = { 0 };

            VkAccelerationStructureBuildSizesInfoKHR tlasBuildSizes{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
            vkGetAccelerationStructureBuildSizesKHR(_device,
                                                    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                    &tlasBuildGeometryInfo,
                                                    tlasMaxPrimitiveCount,
                                                    &tlasBuildSizes);

            getCurrentFrame().tlasBuffer = createBuffer(tlasBuildSizes.accelerationStructureSize,
                                                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);
            
            AllocatedBuffer scratchBuffer{};
            VkDeviceAddress scratchBufferAddress = 0;

            if(tlasBuildSizes.buildScratchSize > 0 ) {
                scratchBuffer = createBuffer(tlasBuildSizes.buildScratchSize, 
                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

                VkBufferDeviceAddressInfo scratchBufferAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
                scratchBufferAddressInfo.buffer = scratchBuffer.buffer;
                scratchBufferAddress = vkGetBufferDeviceAddress(_device, &scratchBufferAddressInfo);
            }

            VkAccelerationStructureCreateInfoKHR tlasCreateInfo{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
            tlasCreateInfo.buffer = getCurrentFrame().tlasBuffer.buffer;
            tlasCreateInfo.offset = 0;
            tlasCreateInfo.size = tlasBuildSizes.accelerationStructureSize;
            tlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

            vkCreateAccelerationStructureKHR(_device, &tlasCreateInfo, nullptr, &getCurrentFrame().tlas);
            tlasBuildGeometryInfo.dstAccelerationStructure = getCurrentFrame().tlas;
            tlasBuildGeometryInfo.scratchData.deviceAddress = scratchBufferAddress;

            VkAccelerationStructureBuildRangeInfoKHR tlasRangeInfo{};
            tlasRangeInfo.primitiveCount = 0;
            tlasRangeInfo.primitiveOffset = 0;
            tlasRangeInfo.firstVertex = 0;
            tlasRangeInfo.transformOffset = 0;

            VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos[] = { &tlasRangeInfo };

            // Build the TLAS
            immediateGraphicsQueueSubmitBlocking([=](VkCommandBuffer cmdBuf){
                vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &tlasBuildGeometryInfo, buildRangeInfos);
            });

            destroyBuffer(scratchBuffer);
        }

        _tlasFramesToUpdate -= 1;
    }
}

void Renderer::immediateGraphicsQueueSubmitBlocking(std::function<void(VkCommandBuffer cmd)>&& function) {
    VK_REQUIRE_SUCCESS(vkResetFences(_device, 1, &_immediateFence));
    VK_REQUIRE_SUCCESS(vkResetCommandBuffer(_immediateCommandBuffer, 0));

    VkCommandBufferBeginInfo beginInfo = init::defaultCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_REQUIRE_SUCCESS(vkBeginCommandBuffer(_immediateCommandBuffer, &beginInfo));

    function(_immediateCommandBuffer);

    VK_REQUIRE_SUCCESS(vkEndCommandBuffer(_immediateCommandBuffer));

    std::vector<VkCommandBuffer> cmdBufs = {_immediateCommandBuffer};
    VkSubmitInfo submitInfo = init::defaultSubmitInfo(cmdBufs);
    vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _immediateFence);

    VK_REQUIRE_SUCCESS(vkWaitForFences(_device, 1, &_immediateFence, true, 9'999'999'999));
}

} //namespace vkrt