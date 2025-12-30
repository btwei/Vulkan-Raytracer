#include "MaterialAsset.hpp"

namespace vkrt
{
    
MaterialAsset::MaterialAsset(const std::string& assetId, MaterialInfo materialInfo, AssetManager* assetManager)
    : Asset(assetId)
    , _assetManager(assetManager)
    , colorFactors(materialInfo.colorFactors)
    , metalRoughnessFactors(materialInfo.metalRoughnessFactors)
    , diffTexture(materialInfo.diffTexture)
    , diffSampler(materialInfo.diffSampler)
    , armTexture(materialInfo.armTexture)
    , armSampler(materialInfo.armSampler)
    , norTexture(materialInfo.norTexture)
    , norSampler(materialInfo.norSampler) { }

MaterialAsset::~MaterialAsset() { }

bool MaterialAsset::onRef() {
    std::vector<AssetHandle<TextureAsset>> loadedHandles;

    if(_assetManager->acquireAsset(diffTexture)) loadedHandles.push_back(diffTexture);
    if(_assetManager->acquireAsset(armTexture)) loadedHandles.push_back(armTexture);
    if(_assetManager->acquireAsset(norTexture)) loadedHandles.push_back(norTexture);

    // If one fails, release all the other references and return false
    if(loadedHandles.size() != 3) {
        for(auto& handle : loadedHandles) _assetManager->releaseAsset(handle);
        return false;
    }

    return true;
}

bool MaterialAsset::onUnref() {
    bool result = true;
    result = _assetManager->releaseAsset(diffTexture) && result;
    result = _assetManager->releaseAsset(armTexture) && result;
    result = _assetManager->releaseAsset(norTexture) && result;

    return result;
}

} // namespace vkrt
