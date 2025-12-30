#include "MeshAsset.hpp"

namespace vkrt {

MeshAsset::MeshAsset(const std::string& assetId, std::span<Vertex>& vertices, std::span<uint32_t>& indices, std::vector<Submesh> meshRanges, Renderer* renderer)
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
    _renderer->enqueueBufferDestruction(_meshBuffers.vertexBuffer, _renderer->getFrameNumber());
    _renderer->enqueueBufferDestruction(_meshBuffers.indexBuffer, _renderer->getFrameNumber());
    return true;
}

} // namespace vkrt