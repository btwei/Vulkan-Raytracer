#include "VulkanContext.hpp"

#include "VulkanInitializers.hpp"

namespace vkrt {

VulkanContext::VulkanContext(Window* window) {
    vkb::Instance vkbInstance = createInstance();
    createSurface();
    createDevice(vkbInstance);
}

VulkanContext::~VulkanContext() {
    // Destroy owned objects in reverse order
    // If a runtime error is thrown, these calls must still be valid
    if(device != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, immediateCommandPool, nullptr);
        vkDestroyFence(device, immediateFence, nullptr);
    }

    vkDestroyDevice(device, nullptr);

    if(instance != VK_NULL_HANDLE) {
        // These require non VK_NULL_HANDLE instances
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkb::destroy_debug_utils_messenger(instance, debugMessenger, nullptr);
    }

    vkDestroyInstance(instance, nullptr);
}

void VulkanContext::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const {
    // Resets fence and buffer
    VK_REQUIRE_SUCCESS(vkResetFences(device, 1, &immediateFence));
    VK_REQUIRE_SUCCESS(vkResetCommandBuffer(immediateCommandBuffer, 0));

    // Submits cmdBuf with given function
    VkCommandBufferBeginInfo beginInfo = init::defaultCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_REQUIRE_SUCCESS(vkBeginCommandBuffer(immediateCommandBuffer, &beginInfo));

    function(immediateCommandBuffer);

    VK_REQUIRE_SUCCESS(vkEndCommandBuffer(immediateCommandBuffer));

    std::vector<VkCommandBuffer> cmdBufs = {immediateCommandBuffer};
    VkSubmitInfo submitInfo = init::defaultSubmitInfo(cmdBufs);
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, immediateFence);

    // Waits until complete
    VK_REQUIRE_SUCCESS(vkWaitForFences(device, 1, &immediateFence, true, 9'999'999'999));
}

vkb::Instance VulkanContext::createInstance() {
    // Initialize Volk; Throws if Vulkan loader cannot be located
    VK_REQUIRE_SUCCESS(volkInitialize());

    // For debug builds, enable validation layers
    #ifndef NDEBUG
        bool bUseValidationLayers = true;
    #else
        bool bUseValidationLayers = false;
    #endif

    // Create a Vulkan instance and store it. Throw a runtime error on failure.
    std::vector<const char*> requiredInstanceExtensions = { VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
                                                            VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME}; 

    vkb::InstanceBuilder instance_builder;
    auto instance_ret = instance_builder.set_app_name("VKRT App")
                                        .set_engine_name("VKRT")
                                        .set_engine_version(1, 0, 0)
                                        .require_api_version(1, 3, 0)
                                        .request_validation_layers(bUseValidationLayers)
                                        .enable_extensions(requiredInstanceExtensions)
                                        .use_default_debug_messenger()
                                        .build();

    if(!instance_ret) throw std::runtime_error("Failed to create Vulkan instance. Error: " + instance_ret.error().message());
    instance = instance_ret.value().instance;
    debugMessenger = instance_ret.value().debug_messenger;

    // Pass instance to Volk
    volkLoadInstance(instance);

    return instance_ret.value();
}

void VulkanContext::createSurface() {
    // Create a Vulkan Surface via SDL3, which handles platform differences
    surface = _window->createSurface(instance);
}

void VulkanContext::createDevice(vkb::Instance& vkbInstance) {
    // Select a physical device and create a logical device with queues
    std::vector<const char*> requiredExtensions = { VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
                                                    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                                                    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                                    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};

    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features13.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainFeatures{};
    swapchainFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
    swapchainFeatures.swapchainMaintenance1 = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    accelerationFeatures.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    rtPipelineFeatures.rayTracingPipeline = VK_TRUE;

    vkb::PhysicalDeviceSelector deviceSelector{vkbInstance};
    auto phys_ret = deviceSelector.set_surface(surface)
                                  .set_required_features_12(features12)
                                  .set_required_features_13(features13)
                                  .add_required_extensions(requiredExtensions)
                                  .add_required_extension_features(swapchainFeatures)
                                  .add_required_extension_features(accelerationFeatures)
                                  .add_required_extension_features(rtPipelineFeatures)
                                  .set_minimum_version(1, 3)
                                  .select();

    if(!phys_ret) throw std::runtime_error("Failed to find compatible Vulkan physical device with required extensions!");
    physicalDevice = phys_ret.value().physical_device;

    vkb::DeviceBuilder deviceBuilder{phys_ret.value()};
    auto device_ret = deviceBuilder.build();
    if(!device_ret) throw std::runtime_error("Failed to create Vulkan device!");
    device = device_ret.value().device;

    // Load device for Volk to reduce dispatch overhead
    volkLoadDevice(device);

    graphicsQueue = device_ret.value().get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamilyIndex = device_ret.value().get_queue_index(vkb::QueueType::graphics).value();
    presentQueue = device_ret.value().get_queue(vkb::QueueType::present).value();
    presentQueueFamilyIndex = device_ret.value().get_queue_index(vkb::QueueType::present).value();
}

void VulkanContext::createImmediateResources() {
    // Initialize immediate command pool
    VkCommandPoolCreateInfo poolInfo = init::defaultCommandPoolInfo(graphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                                                              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    if(vkCreateCommandPool(device, &poolInfo, nullptr, &immediateCommandPool) != VK_SUCCESS) throw std::runtime_error("Failed to create Vulkan command pool!");
    
    VkCommandBufferAllocateInfo allocInfo = init::defaultCommandBufferAllocateInfo(immediateCommandPool);
    vkAllocateCommandBuffers(device, &allocInfo, &immediateCommandBuffer);

    // Initialize immediate fence
    VkFenceCreateInfo fenceInfo = init::defaultFenceInfo();
    VK_REQUIRE_SUCCESS(vkCreateFence(device, &fenceInfo, nullptr, &immediateFence));
}

} // namespace vkrt
