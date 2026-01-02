#include "Renderer.hpp"

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
}

void Renderer::renderScene() {
    // Update BLAS

    // Update TLAS

    // Render with 1 Sample per Pixel

}

void Renderer::cleanup() {
    // Generally, destroy objects in reverse order of creation

    if(_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);

        vmaDestroyAllocator(_allocator);

        // Cleanup frameData
        for(auto& data : _frameData) {
            _frameData->_deletionQueue.flushQueue();

            vkDestroyCommandPool(_device, data._commandPool, nullptr);
            vkDestroyFence(_device, data._renderFence, nullptr);
            vkDestroyFence(_device, data._swapchainFence, nullptr);
            vkDestroySemaphore(_device, data._acquireToRenderSemaphore, nullptr);
            vkDestroySemaphore(_device, data._renderToPresentSemaphore, nullptr);
        }

        vkDestroyCommandPool(_device, _immediateCommandPool, nullptr);
        vkDestroyFence(_device, _immediateFence, nullptr);

        // Cleanup current swapchain
        for(auto& imageView : _swapchainImageViews) {
            vkDestroyImageView(_device, imageView, nullptr);
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
    memcpy(data + vertices.size_bytes(), indices.data(), indices.size_bytes());

    // Create device local buffers and transfer
    meshBuffers.vertexBuffer = createBuffer(vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                                                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | 
                                                             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
                                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    meshBuffers.indexBuffer = createBuffer(indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                           VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    immediateGraphicsQueueSubmit([&](VkCommandBuffer cmdBuf) {
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
    _frameData[(_frameCount - 1) % NUM_FRAMES_IN_FLIGHT]._deletionQueue.pushFunction([=](){
        destroyBuffer(buffer);
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

AllocatedImage Renderer::uploadImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false) {
    size_t dataSize = extent.width * extent.height * extent.depth * 4;
    AllocatedBuffer stagingBuffer = createBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    memcpy(stagingBuffer.info.pMappedData, data, dataSize);

    if(extent.depth != 1) mipmapped = false; // Mipmapping currently doesn't handle 3D textures (and I don't anticipate that I will be using them)
    AllocatedImage newImage = createImage(extent, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    immediateGraphicsQueueSubmit([&](VkCommandBuffer cmdBuf) {
        utils::defaultImageTransition(cmdBuf, newImage.image,
                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                      VK_REMAINING_MIP_LEVELS);

        VkBufferImageCopy bufferRegion{};
        bufferRegion.imageExtent = extent;
        bufferRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferRegion.imageSubresource.baseArrayLayer = 0;
        bufferRegion.imageSubresource.layerCount = 1;
        bufferRegion.imageSubresource.mipLevel = 0;

        vkCmdCopyBufferToImage(cmdBuf, stagingBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferRegion);

        if(mipmapped) {
            uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
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
    _frameData[(_frameCount - 1) % NUM_FRAMES_IN_FLIGHT]._deletionQueue.pushFunction([=](){
        destroyImage(image);
    });
}

void Renderer::initVulkanBootstrap() {
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

    // Create a Vulkan Surface via SDL3, which handles platform differences
    _surface = _window->createSurface(_instance);

    // Select a physical device and create a logical device with queues
    std::vector<const char*> requiredExtensions = { VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
                                                    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME };

    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features13.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainFeatures{};
    swapchainFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
    swapchainFeatures.swapchainMaintenance1 = VK_TRUE;

    vkb::PhysicalDeviceSelector deviceSelector{instance_ret.value()};
    auto phys_ret = deviceSelector.set_surface(_surface)
                                  .set_required_features_12(features12)
                                  .set_required_features_13(features13)
                                  .add_required_extensions(requiredExtensions)
                                  .add_required_extension_features(swapchainFeatures)
                                  .set_minimum_version(1, 3)
                                  .select();

    if(!phys_ret) throw std::runtime_error("Failed to find compatible Vulkan physical device with required extensions!");
    _physicalDevice = phys_ret.value().physical_device;

    vkb::DeviceBuilder deviceBuilder{phys_ret.value()};
    auto device_ret = deviceBuilder.build();
    if(!device_ret) throw std::runtime_error("Failed to create Vulkan device!");
    _device = device_ret.value().device;

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
}

void Renderer::resizeSwapchainResources() {
    // The key to smooth resizing is to not call vkWaitDevice and instead push these to be destroyed later

    // So, push resources into a destruction queue
    // @todo destruction queue

    // And create new swapchain resources -- that straight up overwrite the old values
    createSwapchainResources();
}

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
    for(auto& data : _frameData) {
        VkSemaphoreCreateInfo semaphoreInfo = init::defaultSemaphoreInfo();
        VK_REQUIRE_SUCCESS(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &data._acquireToRenderSemaphore));
        VK_REQUIRE_SUCCESS(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &data._renderToPresentSemaphore));

        VkFenceCreateInfo fenceInfo = init::defaultFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VK_REQUIRE_SUCCESS(vkCreateFence(_device, &fenceInfo, nullptr, &data._renderFence));
        VK_REQUIRE_SUCCESS(vkCreateFence(_device, &fenceInfo, nullptr, &data._swapchainFence));
    }
}

void Renderer::initVMA() {
    VmaAllocatorCreateInfo allocatorCreateInfo{};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice = _physicalDevice;
    allocatorCreateInfo.device = _device;
    allocatorCreateInfo.instance = _instance;
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    vmaCreateAllocator(&allocatorCreateInfo, &_allocator);
}

void Renderer::immediateGraphicsQueueSubmit(std::function<void(VkCommandBuffer cmd)>&& function) {
    VK_REQUIRE_SUCCESS(vkResetFences(_device, 1, &_immediateFence));
    VK_REQUIRE_SUCCESS(vkResetCommandBuffer(_immediateCommandBuffer, 0));

    VkCommandBufferBeginInfo beginInfo = init::defaultCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_REQUIRE_SUCCESS(vkBeginCommandBuffer(_immediateCommandBuffer, &beginInfo));

    function(_immediateCommandBuffer);

    VK_REQUIRE_SUCCESS(vkEndCommandBuffer(_immediateCommandBuffer));

    VkSubmitInfo submitInfo = init::defaultSubmitInfo({_immediateCommandBuffer});
    vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _immediateFence);

    VK_REQUIRE_SUCCESS(vkWaitForFences(_device, 1, &_immediateFence, true, 9'999'999'999));
}

} //namespace vkrt