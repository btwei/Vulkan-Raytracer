#include "Renderer.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include "VulkanInitializers.hpp"
#include "VulkanUtils.hpp"

namespace vkrt {

Renderer::Renderer(Window* window) : _window(window) { }

Renderer::~Renderer() {
    cleanup();
}

void Renderer::init() {
    // Initialize core Vulkan resources
    _vulkanContext = std::make_unique<VulkanContext>(_window);

    // Initialize the swapchain
    VkExtent2D windowExtent(_window->getFramebufferWidth(),
                            _window->getFramebufferHeight());

    _vulkanSwapchain = std::make_unique<SwapchainManager>(*_vulkanContext, windowExtent);

    // Initialize remaining manager classes
    _descriptorManager = std::make_unique<DescriptorManager>(*_vulkanContext);
    _resourceManager = std::make_unique<ResourceManager>(*_vulkanContext);
    _rtManager = std::make_unique<RTManager>(_window, *_vulkanContext, *_descriptorManager, _resourceManager);

    _frameManager = std::make_unique<FrameManager>(*_vulkanContext, *_resourceManager);

    // Handle renderer owned setup
    setViewMatrix(glm::mat4(1));
    setProjectionMatrix(glm::radians(70.f), 10000.f, 0.1f);
}

void Renderer::update() {
    handleResize();

    VK_REQUIRE_SUCCESS(vkWaitForFences(_vulkanCtx->device, 1, &getCurrentFrame()._renderFence, VK_TRUE, 1'000'000'000));

    getCurrentFrame()._deletionQueue.flushQueue();

    // Acquire swapchain image
    uint32_t swapchainImageIndex;
    VkResult imageAcquireResult = vkAcquireNextImageKHR(_vulkanCtx->device, _vulkanSwapchain->swapchain, 1'000'000'000, getCurrentFrame()._acquireToRenderSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);
    if(imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        _shouldResize = true;
        return;
    } else if(imageAcquireResult != VK_SUCCESS && imageAcquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }

    VK_REQUIRE_SUCCESS(vkResetFences(_vulkanCtx->device, 1, &getCurrentFrame()._renderFence));

    resizeDrawImage();

    handleTLASUpdate();

    writeDescriptorUpdates(_vulkanSwapchain->imageViews[swapchainImageIndex]);

    // Do rendering
    VkCommandBuffer cmdBuf = getCurrentFrame()._mainCommandBuffer;

    VK_REQUIRE_SUCCESS(vkResetCommandBuffer(cmdBuf, 0));

    VkCommandBufferBeginInfo beginInfo = vkrt::init::defaultCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_REQUIRE_SUCCESS(vkBeginCommandBuffer(cmdBuf, &beginInfo));

        vkrt::utils::defaultImageTransition(cmdBuf, _vulkanSwapchain->images[swapchainImageIndex],
                                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

        raytraceScene(cmdBuf);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcSubresource.mipLevel = 0;

        blit.srcOffsets[1].x = getCurrentFrame()._drawImage.imageExtent.width;
        blit.srcOffsets[1].y = getCurrentFrame()._drawImage.imageExtent.height;
        blit.srcOffsets[1].z = 1;

        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        blit.dstSubresource.mipLevel = 0;

        blit.dstOffsets[1].x = _vulkanSwapchain->extent.width;
        blit.dstOffsets[1].y = _vulkanSwapchain->extent.height;
        blit.dstOffsets[1].z = 1;

        vkCmdBlitImage(cmdBuf, getCurrentFrame()._drawImage.image, VK_IMAGE_LAYOUT_GENERAL, _vulkanSwapchain->images[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);

        vkrt::utils::defaultImageTransition(cmdBuf, _vulkanSwapchain->images[swapchainImageIndex],
                                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                            VK_ACCESS_TRANSFER_WRITE_BIT, 0,
                                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                            1);

    VK_REQUIRE_SUCCESS(vkEndCommandBuffer(cmdBuf));

    std::vector<VkCommandBuffer> cmdBufs = {cmdBuf};
    std::vector<VkSemaphore> signalSemaphores = {_vulkanSwapchain->swapchainRenderToPresentSemaphores[swapchainImageIndex]};
    std::vector<VkSemaphore> waitSemaphores = {getCurrentFrame()._acquireToRenderSemaphore};
    std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };

    VkSubmitInfo submitInfo = vkrt::init::defaultSubmitInfo(cmdBufs, signalSemaphores, waitSemaphores, waitStages);
    VK_REQUIRE_SUCCESS(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, getCurrentFrame()._renderFence));

    VkPresentInfoKHR presentInfo{ .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.pSwapchains = &_vulkanSwapchain->swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_vulkanSwapchain->swapchainRenderToPresentSemaphores[swapchainImageIndex];

    presentInfo.pImageIndices = &swapchainImageIndex;

    // Use VkSwapchainPresentFence to track whether the current swapchain image is done being presented
    VkSwapchainPresentFenceInfoKHR presentFenceInfo = _vulkanSwapchain->getPresentFenceInfo();
    presentInfo.pNext = &presentFenceInfo;

    // Submit for presentation
    VkResult presentResult = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
    if(presentResult == VK_ERROR_OUT_OF_DATE_KHR) _shouldResize = true;

    _frameCount++;
}

void Renderer::cleanup() {
    // Generally, destroy objects in reverse order of creation

    if(_vulkanContext->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_vulkanContext->device);

        _frameManager.release();

        _rtManager.release();
        _resourceManager.release();
        _descriptorManager.release();
        _vulkanSwapchain.release();
    }

    _vulkanContext.release();
}

UploadedMesh Renderer::uploadMesh(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices, const std::vector<SubmeshInfo>& submeshRanges) {
    UploadedMesh uploadedMesh;
    uploadedMesh.meshBuffers = _resourceManager->uploadMeshBuffer(vertices, indices);
    uploadedMesh.blasInstance = _resourceManager->buildBlas(uploadedMesh.meshBuffers, vertices.size(), indices.size(), );

    return uploadedMesh;
}

void Renderer::unloadMesh(UploadedMesh mesh) {
    _frameManager->getPreviousFrame().deletionQueue.pushFunction([=, this](){
        _resourceManager->destroyBlas(mesh.blasInstance);
        _resourceManager->destroyBuffer(mesh.meshBuffers.indexBuffer);
        _resourceManager->destroyBuffer(mesh.meshBuffers.vertexBuffer);
    });
}

UploadedTexture Renderer::uploadTexture(void* data, uint32_t width, uint32_t height, VkFormat format) {
    UploadedTexture uploadedTexture;
    auto& textureArray = _resourceManager->textureArray; // I opt for auto here because the size of the array may change in the future

    // Find the first available slot and assign it an Allocated Image
    for(size_t i = 0; i < textureArray.size(); i++){
        if(!textureArray[i].has_value()) {
            uploadedTexture.textureIdx = i;
            textureArray[i] = _resourceManager->uploadImage(data, VkExtent3D(width, height, 1),
                                                            format,
                                                            VK_IMAGE_USAGE_SAMPLED_BIT,
                                                            true);
            break;
        }
    }

    return uploadedTexture;
}

void Renderer::unloadTexture(UploadedTexture texture) {
    _frameManager->getPreviousFrame().deletionQueue.pushFunction([=, this](){
        _resourceManager->destroyImage(_resourceManager->textureArray[texture.textureIdx].value());
        _resourceManager->textureArray[texture.textureIdx].reset();
    });
}

void Renderer::setViewMatrix(glm::mat4 viewMatrix) {
    pcs.viewMatrix = viewMatrix;
    pcs.inverseViewMatrix = glm::inverse(viewMatrix);
}

void Renderer::setProjectionMatrix(float fov, float nearPlane, float farPlane) {
    // Cache these values to regenerate the projection matrix on window resize
    _fov = fov;
    _nearPlane = nearPlane;
    _farPlane = farPlane;

    pcs.projectionMatrix = glm::perspective(fov, static_cast<float>(_vulkanSwapchain->extent.width / _vulkanSwapchain->extent.height), nearPlane, farPlane);
    pcs.inverseProjectionMatrix = glm::inverse(pcs.projectionMatrix);
}

void Renderer::refreshProjectionMatrix() {
    pcs.projectionMatrix = glm::perspective(_fov, static_cast<float>(_vulkanSwapchain->extent.width / _vulkanSwapchain->extent.height), _nearPlane, _farPlane);
    pcs.inverseProjectionMatrix = glm::inverse(pcs.projectionMatrix);
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
    imageInfo.imageView = getCurrentFrame()._drawImage.imageView;
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
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::raytraceScene(VkCommandBuffer cmdBuf) {
    // Bind the pipeline
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracingPipeline);

    // Bind the descriptor sets
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracingPipelineLayout, 1, 1, &getCurrentFrame()._descriptorSet1, 0, nullptr);

    // Update and Push constants
    // no updates here yet.. matricies may be updated externally via setViewMatrix or setProjectionMatrix
    vkCmdPushConstants(cmdBuf, _raytracingPipelineLayout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(PushConstants), &pcs);

    // Ray trace
    vkCmdTraceRaysKHR(cmdBuf, &_raygenRegion, &_missRegion, &_hitRegion, &_callableRegion, _vulkanSwapchain->extent.width, _vulkanSwapchain->extent.height, 1);
}

void Renderer::handleResize() {
    // Get resize request from OS
    if(_window->getWasResized()) _shouldResize = true;

    // Process resize if necessary
    if(_shouldResize) {
        // Recreate the swapchain
        VkExtent2D windowExtent = {static_cast<unsigned int>(_window->getFramebufferWidth()),
                                   static_cast<unsigned int>(_window->getFramebufferHeight())};
        _vulkanSwapchain->resize(windowExtent);

        refreshProjectionMatrix();
        
        for(FrameData& data : _frameData) { data._drawImageShouldResize = true; }

        _shouldResize = false;
    }
}

} //namespace vkrt