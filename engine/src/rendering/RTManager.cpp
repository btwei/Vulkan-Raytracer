#include "RTManager.hpp"

#include <cstddef>
#include <filesystem>

#include "VulkanInitializers.hpp"
#include "VulkanUtils.hpp"

namespace vkrt {
    
RTManager::RTManager(Window* window, VulkanContext& vulkanContext, DescriptorManager& descriptorManager, ResourceManager& resourceManager)
: _vulkanContext(vulkanContext)
, _descriptorManager(descriptorManager)
, _resourceManager(resourceManager) {
    initRaytracingPipeline(window);
    initShaderBindingTable();
}

RTManager::~RTManager() {
    _resourceManager.destroyBuffer(sbtBuffer);
    vkDestroyPipeline(_vulkanContext.device, raytracingPipeline, nullptr);
    vkDestroyPipelineLayout(_vulkanContext.device, raytracingPipelineLayout, nullptr);
}

void RTManager::initRaytracingPipeline(Window* window) {
// Load the ray-tracing.spv into a shader module
    std::filesystem::path raytracingSpvPath = std::filesystem::path(window->getBinaryPath()).parent_path() / "shaders" / "ray-tracing.spv";
    VkShaderModule raytracingShaderModule;
    if(!utils::loadShaderModule(raytracingSpvPath, _vulkanContext.device, &raytracingShaderModule)) throw std::runtime_error("Failed to create raytracing shader module!");

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
    pcRange.size = sizeof(RTPushConstants);
    pcRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR; // subject to change as neccessary

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pcRange;

    std::array<VkDescriptorSetLayout, 2> setLayouts = { _descriptorManager.layout0, _descriptorManager.layout1 };

    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();

    VK_REQUIRE_SUCCESS(vkCreatePipelineLayout(_vulkanContext.device, &pipelineLayoutInfo, nullptr, &raytracingPipelineLayout));

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{ .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
    pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    pipelineInfo.pGroups = groups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = 3;
    pipelineInfo.layout = raytracingPipelineLayout;

    VK_REQUIRE_SUCCESS(vkCreateRayTracingPipelinesKHR(_vulkanContext.device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &raytracingPipeline));

    vkDestroyShaderModule(_vulkanContext.device, raytracingShaderModule, nullptr);
}

void RTManager::initShaderBindingTable() {
// Get raytracing device properties to get info on SBT handle size and alignment
    VkPhysicalDeviceProperties2 props{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
    props.pNext = &rtProps;
    vkGetPhysicalDeviceProperties2(_vulkanContext.physicalDevice, &props);
    uint32_t handleSize = rtProps.shaderGroupHandleSize;
    
    // Query rtPipeline for shader group handles
    uint32_t groupCount = 3;
    uint32_t sbtSize = groupCount * handleSize; // groups * bytes / group = # bytes
    std::vector<std::byte> handles(sbtSize); 
    VK_REQUIRE_SUCCESS(vkGetRayTracingShaderGroupHandlesKHR(_vulkanContext.device, raytracingPipeline, 0, groupCount, sbtSize, handles.data()));

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
    AllocatedBuffer stagingBuffer = _resourceManager.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    std::byte* mappedDataPtr = reinterpret_cast<std::byte*>(stagingBuffer.info.pMappedData);

        // Copy our one raygen section
        memcpy(mappedDataPtr + raygenOffset, handles.data(), handleSize);

        // Copy our one miss section
        memcpy(mappedDataPtr + missOffset, handles.data() + handleSize, handleSize);

        // Copy our one hit section
        memcpy(mappedDataPtr + hitOffset, handles.data() + 2 * handleSize, handleSize);

    sbtBuffer = _resourceManager.createBuffer(bufferSize,
                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                              VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
                                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

    _vulkanContext.immediateSubmit([&](VkCommandBuffer cmdBuf){
        VkBufferCopy sbtRegion{};
        sbtRegion.size = bufferSize;
        sbtRegion.srcOffset = 0;
        vkCmdCopyBuffer(cmdBuf, stagingBuffer.buffer, sbtBuffer.buffer, 1, &sbtRegion);
    });

    VkBufferDeviceAddressInfo addressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    addressInfo.buffer = sbtBuffer.buffer;
    sbtAddress = vkGetBufferDeviceAddress(_vulkanContext.device, &addressInfo);

    // Populate regions
    raygenRegion.deviceAddress = sbtAddress;
    raygenRegion.stride = raygenSize;
    raygenRegion.size = raygenSize;

    missRegion.deviceAddress = sbtAddress + missOffset;
    missRegion.stride = missSize;
    missRegion.size = missSize;

    hitRegion.deviceAddress = sbtAddress + hitOffset;
    hitRegion.stride = hitSize;
    hitRegion.size = hitSize;

    callableRegion = {};
}

} // namespace vkrt
