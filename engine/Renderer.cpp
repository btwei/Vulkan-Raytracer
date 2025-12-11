#include "Renderer.hpp"

#include <stdexcept>

namespace vkrt {

Renderer::Renderer(Window* window) : _window(window) {

}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::init() {
    initVulkanBootstrap();
}

void Renderer::submitFrame() {

}

void Renderer::cleanup() {
    // Generally, destroy objects in reverse order of creation
    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkb::destroy_debug_utils_messenger(_instance, _debugMessenger, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

void Renderer::initVulkanBootstrap() {
    // For debug builds, enable validation layers
#ifndef NDEBUG
    bool bUseValidationLayers = true;
#else
    bool bUseValidationLayers = false;
#endif

    // Create a Vulkan instance and store it. Throw a runtime error on failure.
    vkb::InstanceBuilder instance_builder;
    auto instance_ret = instance_builder.set_app_name("VKRT App")
                                        .set_engine_name("VKRT")
                                        .set_engine_version(1, 0, 0)
                                        .require_api_version(1, 3, 0)
                                        .request_validation_layers(bUseValidationLayers)
                                        .use_default_debug_messenger()
                                        .build();

    if(!instance_ret) throw std::runtime_error("Failed to create Vulkan instance. Error: " + instance_ret.error().message());
    _instance = instance_ret.value().instance;
    _debugMessenger = instance_ret.value().debug_messenger;

    // Create a Vulkan Surface via SDL3, which handles platform differences
    _surface = _window->createSurface(_instance);

    // Select a physical device and create a logical device with queues
    vkb::PhysicalDeviceSelector deviceSelector{instance_ret.value()};
    auto phys_ret = deviceSelector.set_surface(_surface)
                                  .set_minimum_version(1, 3)
                                  .select();

    if(!phys_ret) throw std::runtime_error("Failed to find compatible Vulkan physical device with required extensions!");
    _physicalDevice = phys_ret.value().physical_device;

    vkb::DeviceBuilder deviceBuilder{phys_ret.value()};
    auto device_ret = deviceBuilder.build();
    if(!device_ret) throw std::runtime_error("Failed to create Vulkan device!");
    _device = device_ret.value().device;

    _graphicsQueue = device_ret.value().get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamilyIndex = device_ret.value().get_queue_index(vkb::QueueType::graphics).value();
}

} //namespace vkrt