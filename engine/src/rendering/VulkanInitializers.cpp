#include "VulkanInitializers.hpp"

namespace vkrt::init {

VkImageCreateInfo defaultImageInfo(VkExtent3D extent, VkFormat format, VkImageUsageFlags usageFlags) {
    VkImageCreateInfo createInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.extent = extent;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;

    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = usageFlags;

    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //createInfo.queueFamilyIndexCount = 0;
    //createInfo.pQueueFamilyIndices = nullptr;

    //createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // UNDEFINED is 0

    return createInfo;
}

VkImageViewCreateInfo defaultImageViewInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags /* = VK_IMAGE_ASPECT_COLOR_BIT */) {
    VkImageViewCreateInfo createInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.pNext = nullptr;
    createInfo.flags = VkImageViewCreateFlagBits(0);
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.layerCount = 1;
    createInfo.subresourceRange.levelCount = 1;

    return createInfo;
}

VkCommandPoolCreateInfo defaultCommandPoolInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo poolInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = queueFamilyIndex;

    return poolInfo;
}

VkCommandBufferAllocateInfo defaultCommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t count, VkCommandBufferLevel level) {
    VkCommandBufferAllocateInfo allocInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = count;

    return allocInfo;
}

VkFenceCreateInfo defaultFenceInfo(VkFenceCreateFlags flags) {
    VkFenceCreateInfo fenceInfo{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = flags;

    return fenceInfo;
}

VkSemaphoreCreateInfo defaultSemaphoreInfo(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo semaphoreInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    semaphoreInfo.flags = flags;
    
    return semaphoreInfo;
}

VkCommandBufferBeginInfo defaultCommandBufferBeginInfo(VkCommandBufferUsageFlags flags /* = 0 */, const VkCommandBufferInheritanceInfo* pInheritanceInfo /* = VK_NULL_HANDLE */) {
    VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = flags;
    beginInfo.pInheritanceInfo = pInheritanceInfo;

    return beginInfo;
}

VkSubmitInfo defaultSubmitInfo(const std::vector<VkCommandBuffer>& commandBuffers, const std::vector<VkSemaphore>& signalSempahores, const std::vector<VkSemaphore>& waitSempahores, const std::vector<VkPipelineStageFlags>& waitStages) {
    VkSubmitInfo submitInfo{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = commandBuffers.size();
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.signalSemaphoreCount = signalSempahores.size();
    submitInfo.pSignalSemaphores = signalSempahores.data();
    submitInfo.waitSemaphoreCount = waitSempahores.size();
    submitInfo.pWaitSemaphores = waitSempahores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();

    return submitInfo;
}

VkPipelineShaderStageCreateInfo defaultShaderStageInfo(VkShaderStageFlagBits stage, VkShaderModule module, const char* pName) {
    VkPipelineShaderStageCreateInfo shaderStageInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    shaderStageInfo.pNext = nullptr;

    shaderStageInfo.pName = pName;
    shaderStageInfo.stage = stage;
    shaderStageInfo.module = module;

    return shaderStageInfo;
}

VkPipelineShaderStageCreateInfo defaultShaderStageInfo(VkShaderStageFlagBits stage, VkShaderModuleCreateInfo& moduleCreateInfo, const char* pName) {
    VkPipelineShaderStageCreateInfo shaderStageInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    shaderStageInfo.pNext = &moduleCreateInfo;

    shaderStageInfo.pName = pName;
    shaderStageInfo.stage = stage;
    shaderStageInfo.module = VK_NULL_HANDLE;

    return shaderStageInfo;
}

VkRayTracingShaderGroupCreateInfoKHR emptyShaderGroupInfo() {
    VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{ .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
    shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
    shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

    return shaderGroupInfo;
}

}