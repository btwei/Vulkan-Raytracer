#include "MeshAsset.hpp"

namespace vkrt {

MeshAsset::MeshAsset(const std::string& assetId, const std::span<Vertex>& vertices, const std::span<uint32_t>& indices, std::vector<Submesh> meshRanges, Renderer* renderer)
    : Asset(assetId)
    , _renderer(renderer)
    , _vertices(vertices.begin(), vertices.end())
    , _indices(indices.begin(), indices.end())
    , _submeshes(meshRanges) { }

MeshAsset::~MeshAsset() {

}

bool MeshAsset::doLoad() {
    if(!_vertices.empty() && !_indices.empty()) {
        _meshBuffers = _renderer->uploadMesh(_vertices, _indices);
        return true;
    } else {
        return false;
    }
}

bool MeshAsset::doUnload() {
    _renderer->enqueueBufferDestruction(_meshBuffers.vertexBuffer);
    _renderer->enqueueBufferDestruction(_meshBuffers.indexBuffer);
    return true;
}

} // namespace vkrt