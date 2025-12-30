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
    bool result = true;
    result = _assetManager->acquireAsset(diffTexture) && result;
    result = _assetManager->acquireAsset(armTexture) && result;
    result = _assetManager->acquireAsset(norTexture) && result;

    return result;
}

bool MaterialAsset::onUnref() {
    bool result = true;
    result = _assetManager->releaseAsset(diffTexture) && result;
    result = _assetManager->releaseAsset(armTexture) && result;
    result = _assetManager->releaseAsset(norTexture) && result;

    return result;
}

} // namespace vkrt
