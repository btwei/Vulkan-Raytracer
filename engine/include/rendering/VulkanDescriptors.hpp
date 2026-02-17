#ifndef VKRT_VULKANDESCRIPTORS_HPP
#define VKRT_VULKANDESCRIPTORS_HPP

#include "VulkanTypes.hpp"

#include <vector>

namespace vkrt {

struct DescriptorLayoutBuilder {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding(uint32_t bindingSlot, VkDescriptorType type, VkShaderStageFlags stage);
    void clear();
    VkResult build(VkDevice device, VkDescriptorSetLayout* layout, VkDescriptorSetLayoutCreateFlags flags = 0, void* pNext = nullptr);
};

struct DescriptorAllocator {
public:

    VkResult init(VkDevice device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& sizes);
    void resetPool(VkDevice device, VkDescriptorPoolResetFlags flags = 0);
    void destroy(VkDevice device);

    /**
     * @brief Allocates a descriptor set from the allocator's descriptor pool
     */
    VkResult allocate(VkDevice device, VkDescriptorSetLayout* pDescriptorSetLayout, VkDescriptorSet* pDescriptorSet);
private:
    VkDescriptorPool _pool;
};

} // namespace vrkt

#endif // VKRT_VULKANDESCRIPTORS_HPP