#include "FrameManager.hpp"

#include "VulkanInitializers.hpp"
#include "VulkanUtils.hpp"

namespace vkrt {
    
FrameManager::FrameManager(VulkanContext& ctx, ResourceManager& resourceManager, DescriptorManager& descriptorManager, VkExtent2D swapchainExtent)
: _vulkanContext(ctx)
, _device(ctx.device)
, _graphicsQueueFamilyIndex(ctx.graphicsQueueFamilyIndex)
, _resourceManager(resourceManager) 
, _descriptorManager(descriptorManager) {
    initCommandResources();
    initSyncResources();
    initDrawImages(swapchainExtent);
    initDescriptorSets();
    initTlasResources();
}

FrameManager::~FrameManager() {
    // Cleanup frameData
    for(auto& data : frameData) {
        data.deletionQueue.flushQueue();

        _resourceManager.destroyTlas(data.tlasResources);

        // Descriptor sets are destroyed when the pool is destroyed..

        _resourceManager.destroyImage(data.drawImage);

        vkDestroyFence(_device, data.renderFence, nullptr);
        vkDestroySemaphore(_device, data.acquireToRenderSemaphore, nullptr);

        vkDestroyCommandPool(_device, data.commandPool, nullptr);
    }
}

void FrameManager::resizeDrawImages(VkExtent2D swapchainExtent) {
    for(auto& data : frameData) {
        data.resized = true;
        data.resizeExtent = swapchainExtent;
    }
}

void FrameManager::cleanupPerFrame() {
    
}

void FrameManager::initCommandResources() {
    // Initialize per frame command resources
    for(auto& data : frameData) {
        VkCommandPoolCreateInfo poolInfo = init::defaultCommandPoolInfo(_graphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                                                                   VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        if(vkCreateCommandPool(_device, &poolInfo, nullptr, &data.commandPool) != VK_SUCCESS) throw std::runtime_error("Failed to create Vulkan command pool!");
        
        VkCommandBufferAllocateInfo allocInfo = init::defaultCommandBufferAllocateInfo(data.commandPool);
        vkAllocateCommandBuffers(_device, &allocInfo, &data.mainCommandBuffer);
    }
}

void FrameManager::initSyncResources() {
    // Initialize per frame semaphores and fences
    for(auto& data : frameData) {
        VkSemaphoreCreateInfo semaphoreInfo = init::defaultSemaphoreInfo();
        VK_REQUIRE_SUCCESS(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &data.acquireToRenderSemaphore));

        VkFenceCreateInfo fenceInfo = init::defaultFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VK_REQUIRE_SUCCESS(vkCreateFence(_device, &fenceInfo, nullptr, &data.renderFence));
    }
}

void FrameManager::initDrawImages(VkExtent2D swapchainExtent) {
    for(FrameData& data : frameData) {
        data.drawImage = _resourceManager.createImage(VkExtent3D(swapchainExtent.width, swapchainExtent.height, 1),
                                                       VK_FORMAT_R16G16B16A16_SFLOAT,
                                                       VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        
        // Transition the draw image to general for use with raytracing pipelines
        _vulkanContext.immediateSubmit([&](VkCommandBuffer cmdBuf){
            utils::defaultImageTransition(cmdBuf, data.drawImage.image,
                                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                          0, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                                          1);
        });
    }
}

void FrameManager::initDescriptorSets() {
    for(FrameData& data : frameData) {
        data.descriptorSet0 = _descriptorManager.allocateLayout0();
    }
}

void FrameManager::initTlasResources() {
    for(FrameData& data : frameData) {
        std::vector<BlasInstance> emptyTlas;
        data.tlasResources = _resourceManager.buildTlas(emptyTlas);
    }
}

void FrameManager::handleDrawImageResize() {
    FrameData& frameData = getCurrentFrame();

    if(frameData.resized) {
        _resourceManager.destroyImage(frameData.drawImage);
        frameData.drawImage = _resourceManager.createImage(VkExtent3D(frameData.resizeExtent.width, frameData.resizeExtent.height, 1),
                                                   VK_FORMAT_R16G16B16A16_SFLOAT,
                                                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        _vulkanContext.immediateSubmit([&](VkCommandBuffer cmdBuf){
            vkrt::utils::defaultImageTransition(cmdBuf, frameData.drawImage.image,
                                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                                0, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                                                1);
        });

        getCurrentFrame().resized = false;
    }
}

} // namespace vkrt
