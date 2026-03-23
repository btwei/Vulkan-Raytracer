#ifndef VKRT_RTMANAGER_HPP
#define VKRT_RTMANAGER_HPP

#include "DescriptorManager.hpp"
#include "ResourceManager.hpp"
#include "VulkanContext.hpp"
#include "VulkanTypes.hpp"
#include "Window.hpp"

namespace vkrt {

struct RTPushConstants {
    glm::mat4 viewMatrix;
    glm::mat4 inverseViewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 inverseProjectionMatrix;
};

/**
 * @class RTManager
 * @brief Handles Raytracing Vulkan resources
 */    
class RTManager {
public:
    RTManager(Window* window, VulkanContext& vulkanContext, DescriptorManager& descriptorManager,  ResourceManager& resourceManager);
    ~RTManager();

    VkPipelineLayout raytracingPipelineLayout;
    VkPipeline raytracingPipeline;

    AllocatedBuffer sbtBuffer;
    VkDeviceAddress sbtAddress;
    VkStridedDeviceAddressRegionKHR raygenRegion;
    VkStridedDeviceAddressRegionKHR missRegion;
    VkStridedDeviceAddressRegionKHR hitRegion;
    VkStridedDeviceAddressRegionKHR callableRegion;

private:
    VulkanContext& _vulkanContext;
    DescriptorManager& _descriptorManager;
    ResourceManager& _resourceManager;

    void initRaytracingPipeline(Window* window);
    void initShaderBindingTable();
};

} // namespace vkrt

#endif // VKRT_RTMANAGER_HPP