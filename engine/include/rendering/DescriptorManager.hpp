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
};

/**
 * @brief Relatively infrequently updates go to set0 in principle for vkrt
 */
struct Descriptor0 {
    constexpr static DescriptorBinding bindings[] = {
    };
    const static int maxSets = 1;

    VkDescriptorSet descriptorSet;
};

/**
 * @brief Per frame updates go to set1 in principle for vkrt
 */
struct Descriptor1 {
    constexpr static DescriptorBinding bindings[] = {
        {0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR}
    };
    const static int maxSets = 3;

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
    Descriptor1 allocateLayout1();

    VkDescriptorSetLayout layout0;
    VkDescriptorSetLayout layout1;

    VkDescriptorPool pool0;
    VkDescriptorPool pool1;

private:
    VkDevice _device;

    void initDescriptorLayouts();
    void initDescriptorPools();

};

} // namespace vkrt

#endif // VKRT_DESCRIPTORMANAGER_HPP