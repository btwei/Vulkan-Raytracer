#include "ModelAsset.hpp"

namespace vkrt {

ModelAsset::ModelAsset(const std::string& assetId, ModelInfo modelInfo, AssetManager* assetManager)
    : Asset(assetId)
    , _assetManager(assetManager)
    , _mesh(modelInfo.mesh)
    , _materials(modelInfo.materials) { }

ModelAsset::~ModelAsset() { }

bool ModelAsset::onRef() {
    if(!_assetManager->acquireAsset(_mesh)) return false;

    std::vector<AssetHandle<MaterialAsset>> loadedHandles;
    for(AssetHandle<MaterialAsset>& material : _materials) {
        if(_assetManager->acquireAsset(material)) {
            loadedHandles.push_back(material);
        } else {
            // One or more handles failed to load, release all
            _assetManager->releaseAsset(_mesh);
            for(auto& handle : loadedHandles) _assetManager->releaseAsset(handle);
            return false;
        }
    }

    return true;
}

bool ModelAsset::onUnref() {
    bool result = _assetManager->releaseAsset(_mesh);
    for(AssetHandle<MaterialAsset>& material : _materials) {
        if(_assetManager->releaseAsset(material) == false) result = false;
    }

    return result;
}

}