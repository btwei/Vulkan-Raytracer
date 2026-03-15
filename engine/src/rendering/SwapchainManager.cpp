#include "SwapchainManager.hpp"

#include <algorithm>
#include <vector>

#include "VulkanInitializers.hpp"

namespace vkrt {

SwapchainManager::SwapchainManager(const VulkanContext& ctx, VkExtent2D windowExtent)
 : _device(ctx.device)
 , _physicalDevice(ctx.physicalDevice)
 , _surface(ctx.surface)
 , _graphicsQueueFamilyIndex(ctx.graphicsQueueFamilyIndex)
 , _presentQueueFamilyIndex(ctx.presentQueueFamilyIndex) {
    createSwapchain(windowExtent);
}

SwapchainManager::~SwapchainManager() {
    destroySwapchain();
}

void SwapchainManager::resize(VkExtent2D windowExtent) {
    // Cleanup previous resources
    if(_pLatestPresentFence != nullptr) {
        // Using smooth swapchain resizing
        destroySwapchainDeferred();
    } else { 
        // First frame or not using present fences
        vkDeviceWaitIdle(_device);
        destroySwapchain();
    }

    // Create new, resized resources
    createSwapchain(windowExtent, swapchain);
}

VkSwapchainPresentFenceInfoKHR SwapchainManager::getPresentFenceInfo() {
    // To select a free fence, without blocking on any GPU-CPU sync operation,
    // we attempt to get a free fence from the presentFences vector. If none
    // are present we create a new fence and use that one.

    // pLatestPresentFence is used when a resize is handled, to tell us when
    // the old resources are safe to destroy. Small reminder: _pLatestPresentFence
    // must always be updated on vector resize
    VkFence* pPresentFence = nullptr;
    for(VkFence& fence : _presentFences) {
        if(vkGetFenceStatus(_device, fence) == VK_SUCCESS) {
            vkResetFences(_device, 1, &fence);
            pPresentFence = &fence;
            _pLatestPresentFence = &fence;
            break;
        }
    }

    if(pPresentFence == nullptr) {
        VkFence& newFence = _presentFences.emplace_back();
        VkFenceCreateInfo fenceInfo = init::defaultFenceInfo();
        vkCreateFence(_device, &fenceInfo, nullptr, &newFence);
        pPresentFence = &newFence;
        _pLatestPresentFence = &newFence;
    }

    // Create and populate the info struct
    VkSwapchainPresentFenceInfoKHR presentFenceInfo{ 
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR,
        .swapchainCount = 1,
        .pFences = pPresentFence
    };
    
    return presentFenceInfo;
}

void SwapchainManager::cleanupPerFrame() {
    // Attempt to cleanup each oldResource if the fence is signalled
    std::erase_if(_oldResources, [&](OldSwapchainSnapshot& oldResource) {
        if(vkGetFenceStatus(_device, oldResource.presentFence) == VK_SUCCESS) {
            for(int i=0; i < oldResource.imageViews.size(); i++) {
                vkDestroyImageView(_device, oldResource.imageViews[i], nullptr);
                vkDestroySemaphore(_device, oldResource.semaphores[i], nullptr);
            }
            vkDestroySwapchainKHR(_device, swapchain, nullptr);
            vkDestroyFence(_device, oldResource.presentFence, nullptr);

            return true;
        } else {
            return false;
        }
    });
}

void SwapchainManager::setProperExtent(const VkSurfaceCapabilitiesKHR& surfaceCaps, VkExtent2D windowExtent) {
    // Select the actual swapchain extent. 
    if(surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = surfaceCaps.currentExtent;
    } else {
        extent = VkExtent2D{std::clamp<uint32_t>(windowExtent.width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width),
                            std::clamp<uint32_t>(windowExtent.height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height)};
    }
}

void SwapchainManager::setBestFormat() {
    uint32_t surfaceFormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &surfaceFormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &surfaceFormatCount, surfaceFormats.data());

    format = surfaceFormats[0].format;
    colorSpace = surfaceFormats[0].colorSpace;
    for(const auto& surfaceFormat : surfaceFormats) {
        if(surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format = surfaceFormat.format;
            colorSpace = surfaceFormat.colorSpace;
            break;
        }
    } 
}

void SwapchainManager::setBestPresentMode() {
    presentMode = VK_PRESENT_MODE_FIFO_KHR; // Always available
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, presentModes.data());

    for(const auto& mode : presentModes) {
        if(mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
}

void SwapchainManager::createSwapchain(VkExtent2D windowExtent, VkSwapchainKHR oldSwapchain /* = VK_NULL_HANDLE */) {
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &surfaceCaps);

    // Select the actual swapchain extent. 
    setProperExtent(surfaceCaps, windowExtent);

    // Select the best available swapchain format
    setBestFormat();

    // Select the best available presentation mode
    setBestPresentMode();

    // Set image count to the minimum we want, then cap it to the max allowed
    uint32_t imageCount = std::max( 3u, surfaceCaps.minImageCount);
    imageCount = ( surfaceCaps.maxImageCount > 0 && imageCount > surfaceCaps.maxImageCount) ? surfaceCaps.maxImageCount : imageCount;

    VkSwapchainCreateInfoKHR swapchainInfo{ .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchainInfo.oldSwapchain = oldSwapchain;
    swapchainInfo.surface = _surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = format;
    swapchainInfo.imageColorSpace = colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchainInfo.preTransform = surfaceCaps.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = true;

    uint32_t queueFamilyIndices[] = {_graphicsQueueFamilyIndex, _presentQueueFamilyIndex};
    if(_graphicsQueueFamilyIndex != _presentQueueFamilyIndex) {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    if(vkCreateSwapchainKHR(_device, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan swapchain!");
    }

    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(_device, swapchain, &swapchainImageCount, nullptr);
    images.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(_device, swapchain, &swapchainImageCount, images.data());

    imageViews.clear();
    for(int i=0; i<swapchainImageCount; i++) {
        VkImageViewCreateInfo imageViewInfo = init::defaultImageViewInfo(images[i], format);
        VkImageView imageView;
        if(vkCreateImageView(_device, &imageViewInfo, nullptr, &imageView) != VK_SUCCESS) throw std::runtime_error("Failed to create Vulkan swapchain image view!");
        imageViews.push_back(imageView);
    }

    // Create semaphores to guard the swapchain images
    // - the vector containing them must be the same size as the swapchain image count
    swapchainRenderToPresentSemaphores.resize(images.size());
    for(int i = 0; i < images.size(); i++) {
        VkSemaphoreCreateInfo semaphoreInfo = init::defaultSemaphoreInfo();
        VK_REQUIRE_SUCCESS(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &swapchainRenderToPresentSemaphores[i]));
    }
}

void SwapchainManager::destroySwapchain() {
    // Destroy all GPU resources owned by this VulkanSwapchain object
    for(int i = 0; i<imageViews.size(); i++) {
        vkDestroyImageView(_device, imageViews[i], nullptr);
        vkDestroySemaphore(_device, swapchainRenderToPresentSemaphores[i], nullptr);
    }

    vkDestroySwapchainKHR(_device, swapchain, nullptr);

    for(int i = 0; i < _presentFences.size(); i++) {
        vkDestroyFence(_device, _presentFences[i], nullptr);
    }
}

void SwapchainManager::destroySwapchainDeferred() {
    // Remove a non nullptr present fence + associated resources
    _oldResources.push_back({
        *_pLatestPresentFence,
        std::move(imageViews),
        std::move(swapchainRenderToPresentSemaphores),
        swapchain
    });

    // Then replace the present fence that we took out
    VkFenceCreateInfo fenceInfo = vkrt::init::defaultFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    vkCreateFence(_device, &fenceInfo, nullptr, _pLatestPresentFence);
}

} // namespace vkrt