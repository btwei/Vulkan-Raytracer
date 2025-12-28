#ifndef VKRT_VULKANUTILS_HPP
#define VKRT_VULKANUTILS_HPP

#include "VulkanTypes.hpp"

namespace vkrt::utils {
    
void defaultImageTransition(VkCommandBuffer cmdBuf,
                            VkImage image,
                            VkImageLayout oldLayout,
                            VkImageLayout newLayout,
                            VkAccessFlags srcAccessMask,
                            VkAccessFlags dstAccessMask,
                            VkPipelineStageFlags srcStageMask,
                            VkPipelineStageFlags dstStageMask);

}

#endif // VKRT_VULKANUTILS_HPP