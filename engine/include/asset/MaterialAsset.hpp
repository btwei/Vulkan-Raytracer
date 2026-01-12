#ifndef VKRT_MATERIALASSET_HPP
#define VKRT_MATERIALASSET_HPP

#include <glm/glm.hpp>

#include "Asset.hpp"
#include "AssetHandle.hpp"
#include "TextureAsset.hpp"

namespace vkrt {

class MaterialAsset : public Asset {
public:
    struct MaterialInfo {
        glm::vec4 colorFactors;
        glm::vec4 metalRoughnessFactors;

        AssetHandle<TextureAsset> diffTexture;
        AssetHandle<TextureAsset> armTexture;
        AssetHandle<TextureAsset> norTexture;
    };

    MaterialAsset(const std::string& assetId, MaterialInfo materialInfo);
    ~MaterialAsset() override;

private:
    glm::vec4 colorFactors;
    glm::vec4 metalRoughnessFactors;

    AssetHandle<TextureAsset> diffTexture;
    AssetHandle<TextureAsset> armTexture;
    AssetHandle<TextureAsset> norTexture;
};

} // namespace vkrt

#endif // VKRT_MATERIALASSET_HPP