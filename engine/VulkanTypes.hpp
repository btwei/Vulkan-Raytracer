#ifndef VKRT_VULKANTYPES_HPP
#define VKRT_VULKANTYPES_HPP

#include <string>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
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

struct Vertex {
    glm::vec3 position;
    float texCoord0_u;
    glm::vec3 normal;
    float texCoord0_v;
    glm::vec4 color;
    glm::vec4 tangent;
};

struct GPUMeshBuffers {
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;

    VkAccelerationStructureKHR blas;
};

}

#endif