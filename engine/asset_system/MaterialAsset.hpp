#ifndef VKRT_MATERIALASSET_HPP
#define VKRT_MATERIALASSET_HPP

#include "AssetManager.hpp"
#include "TextureAsset.hpp"

namespace vkrt {

class MaterialAsset : public Asset {
public:
    struct MaterialInfo {
        glm::vec4 colorFactors;
        glm::vec4 metalRoughnessFactors;

        AssetHandle<TextureAsset> diffTexture;
        VkSampler diffSampler;
        AssetHandle<TextureAsset> armTexture;
        VkSampler armSampler;
        AssetHandle<TextureAsset> norTexture;
        VkSampler norSampler;
    };

    MaterialAsset(const std::string& assetId, MaterialInfo materialInfo, AssetManager* assetManager);
    ~MaterialAsset() override;
protected:
    virtual bool onRef() override;
    virtual bool onUnref() override;
private:
    AssetManager* _assetManager;

    glm::vec4 colorFactors;
    glm::vec4 metalRoughnessFactors;

    AssetHandle<TextureAsset> diffTexture;
    VkSampler diffSampler;
    AssetHandle<TextureAsset> armTexture;
    VkSampler armSampler;
    AssetHandle<TextureAsset> norTexture;
    VkSampler norSampler;
};

} // namespace vkrt

#endif // VKRT_MATERIALASSET_HPP