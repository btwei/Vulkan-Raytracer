#include "VulkanInitializers.hpp"

namespace vkrt::init {

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

}