#ifndef VKRT_VULKANSWAPCHAIN_HPP
#define VKRT_VULKANSWAPCHAIN_HPP

#include <VulkanTypes.hpp>

namespace vkrt {

struct VulkanSwapchainInfo {
    VkSurfaceKHR surface;
    VkExtent2D extent;
};

/**
 * @class VulkanSwapchain
 * @brief Manages swapchain objects and handles smooth swapchain resizing.
 * 
 * Uses RAII principles. 
 */
class VulkanSwapchain {
public:
    VulkanSwapchain(const DeviceContext& ctx, const VulkanSwapchainInfo& info);
    ~VulkanSwapchain();

    void resize(VkExtent2D windowExtent);
    VkSwapchainPresentFenceInfoKHR getPresentFenceInfo();
    void cleanupPerFrame();

    VkExtent2D extent;
    VkColorSpaceKHR colorSpace;
    VkFormat format;
    VkPresentModeKHR presentMode;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;

    // - Always same size as swapchain images
    // - Safely replaced on resize
    std::vector<VkSemaphore> swapchainRenderToPresentSemaphores;

private:
    struct OldSwapchainSnapshot {
        VkFence presentFence;
        std::vector<VkImageView> imageViews;
        std::vector<VkSemaphore> semaphores;
        VkSwapchainKHR swapchain;
    };

    VkDevice _device;
    VkPhysicalDevice _physicalDevice;
    VkSurfaceKHR _surface;
    uint32_t _graphicsQueueFamilyIndex;
    uint32_t _presentQueueFamilyIndex;

    VkFence* _pLatestPresentFence = nullptr;
    std::vector<VkFence> _presentFences;
    std::vector<OldSwapchainSnapshot> _oldResources;

    void setProperExtent(const VkSurfaceCapabilitiesKHR& surfaceCaps, VkExtent2D windowExtent);
    void setBestFormat();
    void setBestPresentMode();

    void createSwapchain(VkExtent2D windowExtent, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void destroySwapchain();
    void destroySwapchainDeferred();
};

} // namespace vkrt

#endif // VKRT_VULKANSWAPCHAIN_HPP