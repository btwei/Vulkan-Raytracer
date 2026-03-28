#ifndef VKRT_RENDERER_HPP
#define VKRT_RENDERER_HPP

#include <memory>
#include <span>
#include <vector>

#include <volk.h>

#include "DescriptorManager.hpp"
#include "FrameManager.hpp"
#include "RTManager.hpp"
#include "SceneManager.hpp"
#include "VulkanContext.hpp"
#include "VulkanDescriptors.hpp"
#include "SwapchainManager.hpp"
#include "VulkanTypes.hpp"
#include "Window.hpp"

namespace vkrt {

struct UploadedMesh {
    GPUMeshBuffers meshBuffers;
    BlasResources blasInstance;
};

struct UploadedTexture {
    AllocatedImage texture;
    uint32_t textureIdx;
};

struct MaterialParams {
    UploadedTexture albedo;
    UploadedTexture normalMap;
    UploadedTexture metalRoughnessMap;
    UploadedTexture aoMap;
    UploadedTexture emissiveMap;

    glm::vec4 albedoFactor;
    float metallicFactor;
    float roughnessFactor;
    glm::vec3 emissiveFactor;
    float normalScale;
    float occlusionStrength;
};

struct MeshInstance {
    UploadedMesh uploadedMesh;
    glm::mat4 transform;
    uint32_t materialInstance;
};

/**
 * @class Renderer
 * @brief Interfaces with the GPU to handle rendering.
 */
class Renderer {
public:
    Renderer(Window* window);
    ~Renderer();

    void init();
    void update();
    void cleanup();

    UploadedMesh uploadMesh(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices, const std::vector<SubmeshInfo>& submeshRanges);
    void unloadMesh(UploadedMesh mesh);

    UploadedTexture uploadTexture(void* data, uint32_t width, uint32_t height, VkFormat format);
    void unloadTexture(UploadedTexture texture);

    uint32_t uploadMaterialInstance(MaterialParams params);
    void freeMaterialInstance(uint32_t material);

    void setScene(std::unique_ptr<std::vector<MeshInstance>> sceneInstances);

    void setViewMatrix(glm::mat4 viewMatrix);
    void setProjectionMatrix(float fov, float nearPlane, float farPlane);

private:
    bool _isInitialized = false;
    bool _shouldResize = false;

    Window* _window = nullptr;

    std::unique_ptr<VulkanContext> _vulkanContext;
    std::unique_ptr<SwapchainManager> _vulkanSwapchain;
    std::unique_ptr<DescriptorManager> _descriptorManager;
    std::unique_ptr<ResourceManager> _resourceManager;
    std::unique_ptr<RTManager> _rtManager;
    std::unique_ptr<FrameManager> _frameManager;

    // Needed for rebuilding projection matrix on window resize
    float _fov;
    float _nearPlane;
    float _farPlane;

    RTPushConstants pcs;

    void writeDescriptorUpdates(VkImageView swapchainImageView);
    void raytraceScene(VkCommandBuffer cmdBuf);
    void handleResize();

};

} // namespace vkrt

#endif // VKRT_RENDERER_HPP