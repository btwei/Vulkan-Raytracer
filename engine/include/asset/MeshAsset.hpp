#ifndef VKRT_MESHASSET_HPP
#define VKRT_MESHASSET_HPP

#include <span>

#include "Asset.hpp"
#include "Renderer.hpp"
#include "VulkanTypes.hpp"

namespace vkrt {

/**
 * @class MeshAsset
 * 
 * @brief A MeshAsset is derived from the Asset base class. It 
 * contains all of the vertex and index information within a mesh derived
 * from an external asset such as a gltf file. Because there can be multiple
 * sections within a single buffer, there can be several submeshes per MeshAsset.
 */
class MeshAsset : public Asset {
public:
    struct Submesh {
        uint32_t startIndex;
        uint32_t count;
    };

    MeshAsset(const std::string& assetId, const std::span<Vertex>& vertices, const std::span<uint32_t>& indices, std::vector<Submesh> meshRanges, Renderer* renderer);
    ~MeshAsset();
protected:
    virtual bool doLoad() override;
    virtual bool doUnload() override;
private:
    Renderer* _renderer;
    std::vector<Submesh> _submeshes;

    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;

    GPUMeshBuffers _meshBuffers;

    VkAccelerationStructureKHR _blas;
    AllocatedBuffer _scratchBuffer;
};

} // namespace vkrt

#endif // VKRT_MESHASSET_HPP