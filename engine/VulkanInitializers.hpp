#ifndef VKRT_VULKANINITIALIZERS_HPP
#define VKRT_VULKANINITIALIZERS_HPP

#include <vulkan/vulkan.h>

namespace vkrt::init {

VkImageViewCreateInfo defaultImageViewInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

VkCommandPoolCreateInfo defaultCommandPoolInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

VkCommandBufferAllocateInfo defaultCommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

VkFenceCreateInfo defaultFenceInfo(VkFenceCreateFlags flags = 0);

VkSemaphoreCreateInfo defaultSemaphoreInfo(VkSemaphoreCreateFlags flags = 0);

} // namespace vkrt::init

#endif // VKRT_VULKANINITIALIZERS_HPP