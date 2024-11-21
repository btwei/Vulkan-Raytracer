#include "VulkanWrapper.h"

void VulkanWrapper::init(GLFWwindow* window) {
    window_ = window;
}

void VulkanWrapper::cleanup() {
    vkDestroyInstance(instance, nullptr);
};

/* Private Functions */

void VulkanWrapper::createInstance() {

    uint32_t apiVersion = 0;
    if(vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS){
        throw std::runtime_error("failed to get api version");
    }

    uint32_t requiredVersion = VK_MAKE_API_VERSION(0, 1, 2, 162);
    if(apiVersion < requiredVersion) {
        throw std::runtime_error("raytracing requires Vulkan version 1.2.162.0 or greater");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "Vulkan Raytracer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    uint32_t glfwCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwCount);

    std::vector<const char*> requiredExtensions;

    for(uint32_t i = 0; i < glfwCount; i++) {
        requiredExtensions.emplace_back(glfwExtensions[i]);
    }

    requiredExtensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    requiredExtensions.emplace_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    switch(vkCreateInstance(&createInfo, nullptr, &instance)) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            throw std::runtime_error("a required extension is missing!");
        default:
            throw std::runtime_error("failed to create instance");
    }

}