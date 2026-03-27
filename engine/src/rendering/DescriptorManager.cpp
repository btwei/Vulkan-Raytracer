#include "DescriptorManager.hpp"

namespace vkrt {

DescriptorManager::DescriptorManager(VulkanContext& ctx)
: _device(ctx.device) {
    initDescriptorLayouts();
    initDescriptorPools();
}

DescriptorManager::~DescriptorManager() {
    vkDestroyDescriptorPool(_device, pool0, nullptr);

    vkDestroyDescriptorSetLayout(_device, layout0, nullptr);
}

Descriptor0 DescriptorManager::allocateLayout0() {
    Descriptor0 d0;
    VkDescriptorSetAllocateInfo allocInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = pool0;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout0;

    VK_REQUIRE_SUCCESS(vkAllocateDescriptorSets(_device, &allocInfo, &d0.descriptorSet));
    return d0;
}

void DescriptorManager::initDescriptorLayouts() {
    // Create Descriptor Layout 0
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<VkDescriptorBindingFlags> bindingFlags;
        for(const DescriptorBinding& b : Descriptor0::bindings) {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = b.binding;
            binding.descriptorType = b.descriptorType;
            binding.descriptorCount = 1;
            binding.stageFlags = b.stageFlags;
            binding.pImmutableSamplers = nullptr;

            bindings.push_back(binding);
            bindingFlags.push_back(b.bindingFlags);
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
        bindingFlagsInfo.bindingCount = bindingFlags.size();
        bindingFlagsInfo.pBindingFlags = bindingFlags.data();

        VkDescriptorSetLayoutCreateInfo layout0Info{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layout0Info.pNext = &bindingFlagsInfo;
        layout0Info.bindingCount = bindings.size();
        layout0Info.pBindings = bindings.data();

        VK_REQUIRE_SUCCESS(vkCreateDescriptorSetLayout(_device, &layout0Info, nullptr, &layout0));
    }
}

void DescriptorManager::initDescriptorPools() {
    // Create Descriptor Pool 0
    {
        std::vector<VkDescriptorPoolSize> sizes;
        for(const DescriptorBinding b : Descriptor0::bindings) {
            sizes.push_back({ b.descriptorType, b.descriptorCount * Descriptor0::maxSets});
        }

        VkDescriptorPoolCreateInfo poolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.maxSets = Descriptor0::maxSets;
        poolInfo.poolSizeCount = sizes.size();
        poolInfo.pPoolSizes = sizes.data();

        VK_REQUIRE_SUCCESS(vkCreateDescriptorPool(_device, &poolInfo, nullptr, &pool0));
    }
}

} // namespace vkrt