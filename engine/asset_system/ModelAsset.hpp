#ifndef VKRT_MODELASSET_HPP
#define VKRT_MODELASSET_HPP

#include "AssetManager.hpp"
#include "MaterialAsset.hpp"
#include "MeshAsset.hpp"

namespace vkrt {

class ModelAsset : public Asset {
public:
    struct ModelInfo {
        AssetHandle<MeshAsset> mesh;
        std::vector<AssetHandle<MaterialAsset>> materials;
    };

    ModelAsset(const std::string& assetId, ModelInfo modelInfo, AssetManager* assetManager);
    ~ModelAsset() override;
protected:
    virtual bool onRef() override;
    virtual bool onUnref() override;
private:
    AssetManager* _assetManager;

    AssetHandle<MeshAsset> _mesh;
    std::vector<AssetHandle<MaterialAsset>> _materials;
};

} // namespace vkrt

#endif // VKRT_MODELASSET_HPP