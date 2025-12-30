#include "TextureAsset.hpp"

#include <stb_image.h>

namespace vkrt {

TextureAsset::TextureAsset(const std::string& assetId, const std::filesystem::path& filepath, Renderer* renderer)
    : Asset(assetId)
    , _filepath(filepath)
    , _renderer(renderer) { }

TextureAsset::TextureAsset(const std::string& assetId, void* data, VkExtent3D extent, Renderer* renderer)
    : Asset(assetId)
    , _data(data)
    , _extent(extent)
    , _renderer(renderer) { }

TextureAsset::~TextureAsset() {
    unload();
}

bool TextureAsset::doLoad() {
    int x, y, numChannels;

    // data is cached or loaded directly
    if(_data && _extent.width != 0 && _extent.height != 0 && _extent.depth != 0) {
       _texture = _renderer->uploadImage(_data, _extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
        return true;

    // load from filepath
    } else if(!_filepath.empty()) {
        _data = stbi_load(_filepath.c_str(), &x, &y, &numChannels, 4);
        if (!_data) return false; // Failed to create _data via stb_image

        _extent = VkExtent3D{static_cast<uint>(x), static_cast<uint>(y), 1};
        _texture = _renderer->uploadImage(_data, _extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
        stbi_image_free(_data);
        _data = nullptr;
        return true;
    
    // not valid
    } else {
        return false;
    }

}

bool TextureAsset::doUnload() {
    _renderer->enqueueImageDestruction(_texture, _renderer->getFrameNumber());
    return true;
}
    
} // namespace vkrt