#ifndef VKRT_RESOURCEMANAGER_HPP
#define VKRT_RESOURCEMANAGER_HPP

#include "VulkanContext.hpp"
#include "VulkanTypes.hpp"

namespace vkrt {

/**
 * @class ResourceManager
 * @brief Handles GPU resources in the Renderer class
 */
class ResourceManager {
public:
    ResourceManager(const VulkanContext& ctx);
    ~ResourceManager();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    AllocatedBuffer createBuffer();
    GPUMeshBuffers uploadMeshBuffer();
    void destroyBuffer();
    //void enqueueBufferDestruction();
    
    AllocatedImage createImage();
    AllocatedImage uploadImage();
    void destroyImage();
    //void enqueueImageDestruction();

    AllocatedSampler createSampler();
    void destroySampler();

    BlasResources buildBlas();
    void destroyBlas();
    void enqueueBlasDestruction();

    void updateBlas();
    void compactBlas();

    TlasResources buildTlas();
    void destroyTlas();

    uint32_t uploadMeshResources();
    void freeMeshResources();

private:

};

} // namespace vkrt

#endif // VKRT_RESOURCEMANAGER_HPP