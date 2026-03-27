#ifndef VKRT_DESCRIPTORMANAGER_HPP
#define VKRT_DESCRIPTORMANAGER_HPP

#include "VulkanContext.hpp"
#include "VulkanDescriptors.hpp"
#include "VulkanTypes.hpp"

namespace vkrt {

/**
 * @note Bindings must match those in the stader
 */
struct DescriptorBinding {
    uint32_t binding;
    VkDescriptorType descriptorType;
    VkShaderStageFlags stageFlags;
    uint32_t descriptorCount = 1;
    VkDescriptorBindingFlags bindingFlags;
};

/**
 * @note I don't foresee needing multiple descriptor sets at the moment
 */
struct Descriptor0 {
    constexpr static DescriptorBinding bindings[] = {
        {0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 50, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT}
    };
    const static int maxSets = 2;

    VkDescriptorSet descriptorSet;
};

/**
 * @class DescriptorManager
 */
class DescriptorManager {
public:
    DescriptorManager(VulkanContext& ctx);
    ~DescriptorManager();

    Descriptor0 allocateLayout0();

    VkDescriptorSetLayout layout0;
    VkDescriptorPool pool0;

private:
    VkDevice _device;

    void initDescriptorLayouts();
    void initDescriptorPools();

};

} // namespace vkrt

#endif // VKRT_DESCRIPTORMANAGER_HPP