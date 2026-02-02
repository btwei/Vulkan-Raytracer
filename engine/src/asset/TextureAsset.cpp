#include "TextureAsset.hpp"

#include <stb_image.h>

namespace vkrt {

TextureAsset::TextureAsset(const std::string& assetId, const std::filesystem::path& filepath, Renderer* renderer)
    : Asset(assetId)
    , _filepath(filepath)
    , _renderer(renderer) { }

TextureAsset::TextureAsset(const std::string& assetId, const std::vector<unsigned char>& data, VkExtent3D extent, Renderer* renderer)
    : Asset(assetId)
    , _renderer(renderer) {
    _data.reserve(data.size() * sizeof(unsigned char));
    for(unsigned char c : data) {
        _data.push_back(static_cast<std::byte>(c));
    }
}

TextureAsset::TextureAsset(const std::string& assetId, const std::vector<std::byte>& data, VkExtent3D extent, Renderer* renderer)
    : Asset(assetId)
    , _data(data)
    , _extent(extent)
    , _renderer(renderer) { }

TextureAsset::TextureAsset(const std::string& assetId, const std::vector<std::byte>&& data, VkExtent3D extent, Renderer* renderer)
    : Asset(assetId)
    , _data(data)
    , _extent(extent)
    , _renderer(renderer) { }

TextureAsset::~TextureAsset() { }

bool TextureAsset::doLoad() {
    int x, y, numChannels;

    // data is cached or loaded directly
    if(!_data.empty() && _extent.width != 0 && _extent.height != 0 && _extent.depth != 0) {
       _texture = _renderer->uploadImage(_data.data(), _extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
        return true;

    // load from filepath
    } else if(!_filepath.empty()) {
        unsigned char * stbi_data = stbi_load(_filepath.c_str(), &x, &y, &numChannels, 4);
        if (!stbi_data) return false; // Failed to create _data via stb_image

        _extent = VkExtent3D{static_cast<uint32_t>(x), static_cast<uint32_t>(y), 1};
        _texture = _renderer->uploadImage(stbi_data, _extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

        stbi_image_free(stbi_data);
        return true;
    
    // asset was not properly initialized
    } else {
        return false;
    }
}

bool TextureAsset::doUnload() {
    _renderer->enqueueImageDestruction(_texture);
    return true;
}
    
} // namespace vkrt