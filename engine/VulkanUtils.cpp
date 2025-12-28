#include "VulkanUtils.hpp"

namespace vkrt::utils
{
    
void defaultImageTransition(VkCommandBuffer cmdBuf,
                            VkImage image,
                            VkImageLayout oldLayout,
                            VkImageLayout newLayout,
                            VkAccessFlags srcAccessMask,
                            VkAccessFlags dstAccessMask,
                            VkPipelineStageFlags srcStageMask,
                            VkPipelineStageFlags dstStageMask) {
    VkImageMemoryBarrier barrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = image;
    barrier.newLayout = newLayout;
    barrier.oldLayout = oldLayout;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmdBuf,
                         srcStageMask, dstStageMask,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
}

} // namespace vkrt::utils
