#ifndef VKRT_VULKANUTILS_HPP
#define VKRT_VULKANUTILS_HPP

#include "VulkanTypes.hpp"

namespace vkrt::utils {
    
void defaultImageTransition(VkCommandBuffer cmdBuf, VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout);

}

#endif // VKRT_VULKANUTILS_HPP