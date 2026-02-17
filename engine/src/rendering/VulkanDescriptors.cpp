#include "VulkanDescriptors.hpp"

namespace vkrt {

void DescriptorLayoutBuilder::add_binding(uint32_t bindingSlot, VkDescriptorType type, VkShaderStageFlags stage) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = bindingSlot;
    binding.descriptorType = type;
    binding.descriptorCount = 1;
    binding.stageFlags = stage;
    binding.pImmutableSamplers = nullptr;

    bindings.push_back(binding);
}

void DescriptorLayoutBuilder::clear() {
    bindings.clear();
}

VkResult DescriptorLayoutBuilder::build(VkDevice device, VkDescriptorSetLayout* layout, VkDescriptorSetLayoutCreateFlags flags /* = 0*/, void* pNext /* = nullptr */) {
    VkDescriptorSetLayoutCreateInfo layoutInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.pNext = pNext;
    layoutInfo.flags = flags;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    return vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, layout);
}

VkResult DescriptorAllocator::init(VkDevice device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& sizes) {
    VkDescriptorPoolCreateInfo poolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = sizes.size();
    poolInfo.pPoolSizes = sizes.data();

    return vkCreateDescriptorPool(device, &poolInfo, nullptr, &_pool);
}

void DescriptorAllocator::resetPool(VkDevice device, VkDescriptorPoolResetFlags flags /* = 0 */) {
    vkResetDescriptorPool(device, _pool, flags);
}

void DescriptorAllocator::destroy(VkDevice device) {
    vkDestroyDescriptorPool(device, _pool, nullptr);
}

VkResult DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout* pDescriptorSetLayout, VkDescriptorSet* pDescriptorSet) {
    VkDescriptorSetAllocateInfo allocInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = _pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = pDescriptorSetLayout;

    return vkAllocateDescriptorSets(device, &allocInfo, pDescriptorSet);
}

} // namespace vkrt