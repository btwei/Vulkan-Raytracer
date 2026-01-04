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
                            VkPipelineStageFlags dstStageMask,
                            uint32_t levelCount = 1);

/**
 * @note Requires all mipLevels to be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL first
 */
void generateMipmaps(VkCommandBuffer cmdBuf, VkImage image, VkExtent2D extent, uint32_t mipLevels);

}

#endif // VKRT_VULKANUTILS_HPP