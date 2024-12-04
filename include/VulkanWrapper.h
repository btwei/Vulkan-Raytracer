#ifndef VULKAN_WRAPPER_H
#define VULKAN_WRAPPER_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <cstring>

class VulkanWrapper {
public:
    void init(GLFWwindow* window);
    void cleanup();
private:
    static const std::vector<const char*> validationLayers;
    
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    GLFWwindow* window_ = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    void createInstance();
    void setupDebugMessenger();

    /* Helper Functions - Instance Creation */
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();

    /* Helper Functions - Debug Messenger */
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance,VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
};

#endif