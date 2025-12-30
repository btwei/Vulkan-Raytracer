#ifndef VKRT_TEXTUREASSET_HPP
#define VKRT_TEXTUREASSET_HPP

#include <filesystem>

#include "AssetManager.hpp"
#include "Renderer.hpp"

namespace vkrt {

class TextureAsset : public Asset {
public:
    TextureAsset(const std::string& assetId, const std::filesystem::path& filepath, Renderer* renderer);
    TextureAsset(const std::string& assetId, void* data, VkExtent3D extent, Renderer* renderer);
    ~TextureAsset() override;
protected:
    virtual bool doLoad() override;
    virtual bool doUnload() override;
private:
    const std::filesystem::path _filepath;
    Renderer* _renderer;

    void* _data = nullptr;
    VkExtent3D _extent;

    AllocatedImage _texture;
};

} // namespace vkrt

#endif // VKRT_TEXTUREASSET_HPP