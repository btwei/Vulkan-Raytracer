#ifndef VKRT_VULKANINITIALIZERS_HPP
#define VKRT_VULKANINITIALIZERS_HPP

#include <vulkan/vulkan.h>

namespace vkrt::init {

VkImageViewCreateInfo defaultImageViewInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

} // namespace vkrt::init

#endif // VKRT_VULKANINITIALIZERS_HPP