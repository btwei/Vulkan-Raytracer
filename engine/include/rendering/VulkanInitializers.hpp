#ifndef VKRT_VULKANINITIALIZERS_HPP
#define VKRT_VULKANINITIALIZERS_HPP

#include <vector>

#include <volk.h>

namespace vkrt::init {

VkImageCreateInfo defaultImageInfo(VkExtent3D extent, VkFormat format, VkImageUsageFlags usageFlags);

VkImageViewCreateInfo defaultImageViewInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

VkCommandPoolCreateInfo defaultCommandPoolInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

VkCommandBufferAllocateInfo defaultCommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

VkFenceCreateInfo defaultFenceInfo(VkFenceCreateFlags flags = 0);

VkSemaphoreCreateInfo defaultSemaphoreInfo(VkSemaphoreCreateFlags flags = 0);

VkCommandBufferBeginInfo defaultCommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0, const VkCommandBufferInheritanceInfo* pInheritanceInfo = VK_NULL_HANDLE);

/**
 * @note vector references must remain valid through vkQueueSubmit() call
 */
VkSubmitInfo defaultSubmitInfo(const std::vector<VkCommandBuffer>& commandBuffers, const std::vector<VkSemaphore>& signalSempahores = {}, const std::vector<VkSemaphore>& waitSempahores = {}, const std::vector<VkPipelineStageFlags>& waitStages = {});

VkPipelineShaderStageCreateInfo defaultShaderStageInfo(VkShaderStageFlagBits stage, VkShaderModule module, const char* pName);
VkPipelineShaderStageCreateInfo defaultShaderStageInfo(VkShaderStageFlagBits stage, VkShaderModuleCreateInfo& moduleCreateInfo, const char* pName);

VkRayTracingShaderGroupCreateInfoKHR emptyShaderGroupInfo();

} // namespace vkrt::init

#endif // VKRT_VULKANINITIALIZERS_HPP