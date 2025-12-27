#include "VulkanUtils.hpp"

namespace vkrt::utils
{
    
void defaultImageTransition(VkCommandBuffer cmdBuf,
                            VkImage image,
                            VkImageLayout srcLayout,
                            VkImageLayout dstLayout,
                            VkAccessFlags srcAccessFlags,
                            VkAccessFlags dstAccessFlags,
                            VkPipelineStageFlags srcPipelineFlags,
                            VkPipelineStageFlags dstPipelineFlags) {
    VkImageMemoryBarrier barrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = image;
    barrier.newLayout = dstLayout;
    barrier.oldLayout = srcLayout;
    barrier.srcAccessMask = srcAccessFlags;
    barrier.dstAccessMask = dstAccessFlags;
    barrier.subresourceRange.aspectMask = ;

    vkCmdPipelineBarrier(cmdBuf, srcPipelineFlags, dstPipelineFlags, );
}

} // namespace vkrt::utils
