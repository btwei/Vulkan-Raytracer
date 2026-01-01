#include "AssetManager.hpp"

#include <regex>

#include <glm/gtx/transform.hpp>

#include "TextureAsset.hpp"

namespace vkrt {

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
    ImportResult r;

    // TODO: implement importer classes (e.g. GltfImporter)

    return r;
}

template<typename T>
AssetHandle<T> AssetManager::fetchHandleById(const std::string& assetId) {
    auto& assetsOfTypeT = assets[std::type_index(typeid(T))];
    auto it = assetsOfTypeT.find(assetId);

    if(it != assetsOfTypeT.end()) {
        return AssetHandle<T>(assetId, this);
    } else {
        return AssetHandle<T>();
    }
}

template<typename T>
bool acquireAsset(AssetHandle<T> assetHandle) {
    // Find the asset
    auto& assetsOfTypeT = assets[std::type_index(typeid(T))];
    auto it = assetsOfTypeT.find(assetHandle.getId());

    // Process the onRef call
    if(it != assetsOfTypeT.end() && it->asset->onRef()) {
        // If refCount was 0, process the load call
        if(it->refCount == 0){
            // Attempt to load the asset
            if(it->asset->load() == false) return false;
        }
        it->refCount += 1;
        return true;
    } else {
        // Return false if the asset was not found
        return false;
    }
}

template<typename T>
bool releaseAsset(AssetHandle<T> assetHandle) {
    auto& refCountsForTypeT = refCounts[std::type_index(typeid(T))];
    auto it = refCountsForTypeT.find(assetHandle.getId());

    if(it != refCountsForTypeT.end() && it->asset->doUnref()) {
        if(it->refCount == 0) {
             if(it->asset->unload() == false) return false;
        }
        it->refCount -= 1;
        return true;
    } else {
        return false;
    }
}

template<typename T>
AssetHandle<T> AssetManager::registerAsset(std::shared_ptr<T> asset) {
    Asset a = dynamic_cast<Asset>(*asset);
    if(!a) return AssetHandle<T>();

    auto& assetsOfTypeT = assets[std::type_index(typeid(T))];

    // Attempt to place the asset in the asset manager
    while(true) {
        auto it = assetsOfTypeT.find(a.getId());
        // No conflicting ID
        if(it != assetsOfTypeT.end()) {
            assetsOfTypeT[a.getId()] = AssetData(std::move(asset), 0);
            return AssetHandle<T>(a.getId(), this);
        
        // Must add a suffix to generate a unique ID
        } else {
            std::string newId;
            std::regex r(R"(^(.*_)(\d+)$)");
            std::smatch match;
            if(std::regex_search(a.getId(), match, r)) {
                newId = match[1] + std::to_string(std::stoi(match[2]+1));
            } else {
                newId = a.getId() + "_1";
            }
            a.setId(newId);
        }
    }
}

}