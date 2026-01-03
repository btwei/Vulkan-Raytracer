#include "AssetManager.hpp"

#include <regex>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "GltfImporter.hpp"
#include "TextureAsset.hpp"

namespace vkrt {

AssetManager::AssetManager(Renderer* renderer)
    : _renderer(renderer) {

}

AssetManager::~AssetManager() {
    
}

void AssetManager::init() {
    // Initialize default assets
    // Load a default white texture
    std::vector<std::byte> textureData(4);
    uint32_t whiteData = glm::packUnorm4x8(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    memcpy(textureData.data(), &whiteData, sizeof(whiteData));

    registerAsset(std::make_shared<TextureAsset>("whiteTexture", textureData, VkExtent3D(1, 1, 1), _renderer));

    // Load a default grey texture
    uint32_t greyData = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1.0f));
    memcpy(textureData.data(), &greyData, sizeof(greyData));

    registerAsset(std::make_shared<TextureAsset>("greyTexture", textureData, VkExtent3D(1, 1, 1), _renderer));

    // Load a default black texture
    uint32_t blackData = glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 1));
    memcpy(textureData.data(), &blackData, sizeof(blackData));

    registerAsset(std::make_shared<TextureAsset>("blackTexture", textureData, VkExtent3D(1, 1, 1), _renderer));

    // Load an error texture
    uint32_t magentaData = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    textureData.reserve(16*16*4);
    textureData.clear();
    for(int x=0; x < 16; x++) {
        for(int y=0; y < 16; y++) {
            if(((x%2) ^ (y %2))) {
                memcpy(textureData.data() + x*16*4+y*4, &magentaData, sizeof(magentaData));
            } else {
                memcpy(textureData.data() + x*16*4+y*4, &blackData, sizeof(blackData));
            }
        }
    }

    registerAsset(std::make_shared<TextureAsset>("errorCheckboardTexture", textureData, VkExtent3D(16, 16, 1), _renderer));
}

ImportResult AssetManager::importAsset(const std::filesystem::path& filepath) {
    if(filepath.extension() == ".gltf" || filepath.extension() == ".glb") {
        return importGLTF(filepath, this);
    } else {
        return ImportResult();
    }
}

}