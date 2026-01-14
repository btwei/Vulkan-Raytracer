#ifndef VKRT_ASSETMANAGER_HPP
#define VKRT_ASSETMANAGER_HPP

#include <filesystem>
#include <regex>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "Asset.hpp"
#include "AssetHandle.hpp"
#include "ModelAsset.hpp"
#include "Renderer.hpp"
#include "VulkanTypes.hpp"

namespace vkrt {

class MeshAsset;
class MaterialAsset;
class TextureAsset;

struct ImportResult {
    bool success = false;

    std::vector<AssetHandle<ModelAsset>> modelHandles;
    std::vector<AssetHandle<MeshAsset>> meshHandles;
    std::vector<AssetHandle<MaterialAsset>> materialHandles;
    std::vector<AssetHandle<TextureAsset>> textureHandles;
};

class AssetManager {
private:
    struct AssetData {
        std::shared_ptr<Asset> asset;
        int refCount;
    };

    Renderer* _renderer;

    std::unordered_map<std::type_index, std::unordered_map<std::string, AssetData>> assetDatas;

    ImportResult loadGLTF(const std::filesystem::path& filepath);

    // Interal asset release method for use with pairs instead of handles
    bool releaseAsset(std::pair<std::type_index, std::string> pair) {
        auto& refCountsForTypeT = assetDatas[pair.first];
        auto it = refCountsForTypeT.find(pair.second);

        if(it != refCountsForTypeT.end()) {
            for(std::pair<std::type_index, std::string>& prereqPair : it->second.asset->getReferenceList()) {
                releaseAsset(prereqPair);
            }

            if(it->second.refCount == 0) {
                 if(it->second.asset->unload() == false) return false;
            }
            it->second.refCount -= 1;
            return true;
        } else {
            return false;
        }
    }

    // Internal asset acquisition method for use with pairs instead of handles
    bool acquireAsset(std::pair<std::type_index, std::string> pair) {
        // Find the asset
        auto& assetsOfTypeT = assetDatas[pair.first];
        auto it = assetsOfTypeT.find(pair.second);

        // Process the onRef call
        if(it != assetsOfTypeT.end()) {
            // Acquire all referenced assets
            std::vector<std::pair<std::type_index, std::string>> previouslyAdded;
            for(std::pair<std::type_index, std::string>& prereqPair : it->second.asset->getReferenceList()) {
                if(!acquireAsset(prereqPair)) {
                    // rewind ref counts
                    for(std::pair<std::type_index, std::string> unwindPair : previouslyAdded) {
                        releaseAsset(unwindPair);
                    }
                    return false;
                } else {
                    previouslyAdded.push_back(prereqPair);
                }
            }

            // If refCount was 0, process the load call
            if(it->second.refCount == 0){
                // Attempt to load the asset
                if(it->second.asset->load() == false) return false;
            }
            it->second.refCount += 1;
            return true;
        } else {
            // Return false if the asset was not found
            return false;
        }
    }

public:
    AssetManager(Renderer* renderer);
    ~AssetManager();

    void init();
    ImportResult importAsset(const std::filesystem::path& filepath);

    template<typename T>
    AssetHandle<T> getHandleById(const std::string& assetId) {
        auto& assetsOfTypeT = assetDatas[std::type_index(typeid(T))];
        auto it = assetsOfTypeT.find(assetId);

        if(it != assetsOfTypeT.end()) {
            // Restore the type from asset to type T
            return AssetHandle<T>(assetId, std::dynamic_pointer_cast<T>(it->second.asset));
        } else {
            return AssetHandle<T>();
        }
    }

    template<typename T>
    bool hasAsset(const std::string& assetId) {
        auto& assetsOfTypeT = assetDatas[std::type_index(typeid(T))];
        auto it = assetsOfTypeT.find(assetId);

        if(it != assetsOfTypeT.end()) {
            return true;
        } else {
            return false;
        }
    }

    template<typename T>
    T* getAsset(const std::string& assetId) {
        auto& assetsOfTypeT = assetDatas[std::type_index(typeid(T))];
        auto it = assetsOfTypeT.find(assetId);

        if(it != assetsOfTypeT.end()) {
            return it->second.asset;
        } else {
            return nullptr;
        }
    }

    template<typename T>
    bool acquireAsset(AssetHandle<T> assetHandle) {
        // Find the asset
        auto& assetsOfTypeT = assetDatas[std::type_index(typeid(T))];
        auto it = assetsOfTypeT.find(assetHandle.getId());

        // Process the onRef call
        if(it != assetsOfTypeT.end()) {
            // Acquire all referenced assets
            std::vector<std::pair<std::type_index, std::string>> previouslyAdded;
            for(std::pair<std::type_index, std::string>& prereqPair : it->second.asset->getReferenceList()) {
                if(!acquireAsset(prereqPair)) {
                    // rewind ref counts
                    for(std::pair<std::type_index, std::string> unwindPair : previouslyAdded) {
                        releaseAsset(unwindPair);
                    }
                    return false;
                } else {
                    previouslyAdded.push_back(prereqPair);
                }
            }

            // If refCount was 0, process the load call
            if(it->second.refCount == 0){
                // Attempt to load the asset
                if(it->second.asset->load() == false) return false;
            }
            it->second.refCount += 1;
            return true;
        } else {
            // Return false if the asset was not found
            return false;
        }
    }

    template<typename T>
    bool releaseAsset(AssetHandle<T> assetHandle) {
        auto& refCountsForTypeT = assetDatas[std::type_index(typeid(T))];
        auto it = refCountsForTypeT.find(assetHandle.getId());

        if(it != refCountsForTypeT.end()) {
            for(std::pair<std::type_index, std::string>& prereqPair : it->second.asset->getReferenceList()) {
                releaseAsset(prereqPair);
            }

            if(it->second.refCount == 0) {
                 if(it->second.asset->unload() == false) return false;
            }
            it->second.refCount -= 1;
            return true;
        } else {
            return false;
        }
    }

    template<typename T>
    AssetHandle<T> registerAsset(std::shared_ptr<T> asset) {
        std::shared_ptr<Asset> a = std::dynamic_pointer_cast<Asset>(asset);
        if(!a) return AssetHandle<T>();

        auto& assetsOfTypeT = assetDatas[std::type_index(typeid(T))];

        // Attempt to place the asset in the asset manager
        while(true) {
            auto it = assetsOfTypeT.find(a->getId());
            // No conflicting ID
            if(it == assetsOfTypeT.end()) {
                std::string id = a->getId();
                assetsOfTypeT[a->getId()] = AssetData();
                assetsOfTypeT[a->getId()].asset = std::move(a);

                // Restore the type from Asset to type T
                return AssetHandle<T>(id, std::dynamic_pointer_cast<T>(assetsOfTypeT[id].asset));
            
            // Must add a suffix to generate a unique ID
            } else {
                std::string newId;
                std::regex r(R"(^(.*_)(\d+)$)");
                std::smatch match;
                if(std::regex_search(a->getId(), match, r)) {
                    newId = match[1].str() + std::to_string(std::stoi(match[2].str())+1);
                } else {
                    newId = a->getId() + "_1";
                }
                a->setId(newId);
            }
        }
    }

    Renderer* getRenderer() { return _renderer; }
};

} // namespace vkrt

#endif // VKRT_ASSETMANAGER_HPP