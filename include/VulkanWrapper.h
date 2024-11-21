#ifndef VULKAN_WRAPPER_H
#define VULKAN_WRAPPER_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <vector>

class VulkanWrapper {
public:
    void init(GLFWwindow* window);
    void cleanup();
private:
    GLFWwindow* window_ = nullptr;
    VkInstance instance = VK_NULL_HANDLE;

    void createInstance();
};

#endif