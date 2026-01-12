#include "MaterialAsset.hpp"

namespace vkrt
{
    
MaterialAsset::MaterialAsset(const std::string& assetId, MaterialInfo materialInfo)
    : Asset(assetId)
    , colorFactors(materialInfo.colorFactors)
    , metalRoughnessFactors(materialInfo.metalRoughnessFactors)
    , diffTexture(materialInfo.diffTexture)
    , armTexture(materialInfo.armTexture)
    , norTexture(materialInfo.norTexture) {
        _referenceList.push_back({std::type_index(std::type_index(typeid(TextureAsset))), materialInfo.armTexture.getId()});
        _referenceList.push_back({std::type_index(std::type_index(typeid(TextureAsset))), materialInfo.diffTexture.getId()});
        _referenceList.push_back({std::type_index(std::type_index(typeid(TextureAsset))), materialInfo.norTexture.getId()});
    }

MaterialAsset::~MaterialAsset() { }

} // namespace vkrt
