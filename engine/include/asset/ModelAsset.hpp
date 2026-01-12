#ifndef VKRT_MODELASSET_HPP
#define VKRT_MODELASSET_HPP

#include "Asset.hpp"
#include "AssetHandle.hpp"
#include "MaterialAsset.hpp"
#include "MeshAsset.hpp"

namespace vkrt {

class AssetManager;

class ModelAsset : public Asset {
public:
    struct ModelInfo {
        AssetHandle<MeshAsset> mesh;
        std::vector<AssetHandle<MaterialAsset>> materials;
    };

    ModelAsset(const std::string& assetId, ModelInfo modelInfo);
    ~ModelAsset() override;
private:
    AssetHandle<MeshAsset> _mesh;
    std::vector<AssetHandle<MaterialAsset>> _materials;
};

} // namespace vkrt

#endif // VKRT_MODELASSET_HPP