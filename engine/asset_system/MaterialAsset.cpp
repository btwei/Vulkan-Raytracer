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
    _assetManager->acquireAsset(diffTexture);
    _assetManager->acquireAsset(armTexture);
    _assetManager->acquireAsset(norTexture);
}

bool MaterialAsset::onUnref() {
    _assetManager->releaseAsset(diffTexture);
    _assetManager->releaseAsset(armTexture);
    _assetManager->releaseAsset(norTexture);
}

} // namespace vkrt
