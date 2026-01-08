#ifndef VKRT_VULKANTYPES_HPP
#define VKRT_VULKANTYPES_HPP

#include <string>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/hash.hpp>

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

    bool operator==(const Vertex& other) const {
        return position == other.position &&
               texCoord0_u == other.texCoord0_u &&
               normal == other.normal &&
               texCoord0_v == other.texCoord0_v &&
               color == other.color &&
               tangent == other.tangent;
    }
};

struct GPUMeshBuffers {
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;

    VkAccelerationStructureKHR blas;
};

}

inline void hash_combine(std::size_t& seed, std::size_t value) noexcept {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

namespace std {
    template <> struct hash<vkrt::Vertex> {
        size_t operator()(vkrt::Vertex const& vertex) const {
            std::size_t h1 = hash<glm::vec3>()(vertex.position);
            std::size_t h2 = hash<glm::vec3>()(vertex.normal);
            std::size_t h3 = hash<glm::vec3>()(vertex.color);
            std::size_t h4 = hash<glm::vec4>()(vertex.tangent);
            std::size_t h5 = hash<glm::vec2>()({vertex.texCoord0_u, vertex.texCoord0_v});

            std::size_t seed = 0;
            hash_combine(seed, h1);
            hash_combine(seed, h2);
            hash_combine(seed, h3);
            hash_combine(seed, h4);
            hash_combine(seed, h5);

            return seed;
        }
    };
}

#endif