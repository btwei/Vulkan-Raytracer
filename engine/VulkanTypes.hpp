#ifndef VKRT_VULKANTYPES_HPP
#define VKRT_VULKANTYPES_HPP

#include <string>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

namespace vkrt {

#define VK_REQUIRE_SUCCESS( func)                                                            \
    {                                                                                        \
        VkResult e = func;                                                                   \
        if(e != VK_SUCCESS) {                                                                \
            throw std::runtime_error(std::string(#func) + " failed with error: "+ string_VkResult(e));     \
        }                                                                                    \
    }                                                                                        \

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

}

#endif