#ifndef VKRT_ASSETMANAGER_HPP
#define VKRT_ASSETMANAGER_HPP

#include <filesystem>
#include <regex>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
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

/**
 * @brief ImportResults contain AssetHandles on a successful file import.
 */
struct ImportResult {
    bool success = false;

    std::vector<AssetHandle<ModelAsset>> modelHandles;
    std::vector<AssetHandle<MeshAsset>> meshHandles;
    std::vector<AssetHandle<MaterialAsset>> materialHandles;
    std::vector<AssetHandle<TextureAsset>> textureHandles;
};

/**
 * @class IAssetManager
 * @brief This class provides the engine exposed functions to import assets.
 */
class IAssetManager {
public:
    /**
     * @brief Imports the asset at the target filepath.
     * 
     * Parses the file extension to call the correct loading function. Filepaths
     * must point to a resource relative to the assets/ folder.
     * 
     * @param[in] filepath The relative path to the asset file.
     * @return Returns an ImportResult struct with all parsed handles.
     * 
     * @example importAsset("horse_statue_01_8k/horse_statue_01_8k.gltf")
     */
    virtual ImportResult importAsset(const std::filesystem::path& filepath) = 0;

    /**
     * @brief This function loads a GLTF file from the target filepath.
     * 
     * Loads the .gltf or .glb file from the target filepath relative to the assets/
     * folder.
     * 
     * @param[in] filepath The relative path to the gltf file.
     * @return Returns an ImportResult struct with all parsed handles.
     * 
     * @return This returns an ImportResult that is populated with AssetHandles
     */
    virtual ImportResult importGLTF(const std::filesystem::path& filepath) = 0;
};

/**
 * @class AssetManager
 * @brief This class contains the engine implementation of the AssetManager class.
 * 
 * This class is responsible for importing asset files into AssetHandles. Additionally,
 * this class serves as the engine's internal interface for transitioning AssetHandle
 * states to initialize, load, and unload assets.
 */
class AssetManager final : public IAssetManager {
public:
    AssetManager(Renderer* renderer);
    ~AssetManager();

    /**
     * @brief Initializes the AssetManager class. 
     * 
     * Must be called before using the AssetManager. Initializes the binary filepath
     * and default assets for use with engine and importing. The binary path 
     */
    void init(const std::string& binaryPath);

    /**
     * @brief Imports the asset at the target filepath by calling the correct import
     * function.
     * 
     * @example importAsset("horse_statue_01_8k/horse_statue_01_8k.gltf")
     */
    ImportResult importAsset(const std::filesystem::path& filepath) override;

    /**
     * @brief Imports the .glb or .gltf asset at the target path.
     */
    ImportResult importGLTF(const std::filesystem::path& filepath) override;

    Renderer* getRenderer() { return _renderer; }

// More public functions afterwards due to templated code
private:
    /**
     * For each registered asset, hold ownership of it and reference count it.
     */
    struct AssetData {
        std::unique_ptr<Asset> asset;
        int refCount;
    };

    Renderer* _renderer;
    std::string _binaryPath;

    // For every Asset type, store an unordered map of Assets of that kind
    std::unordered_map<std::type_index, std::unordered_map<std::string, AssetData>> assetDatas;

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
    /**
     * @brief Register an object derived from Asset to the Asset Manager.
     * 
     * On asset registration, the name of the asset may be changed if an asset
     * already exists with the same name. The new name can be checked through the
     * returned asset handle.
     * 
     * @param[in] asset An asset object derived from Asset class to be registered.
     * @return An asset handle refering to the registered object.
     */
    template<typename T>
    AssetHandle<T> registerAsset(std::unique_ptr<T> asset) {
        // Check if T is derived from Asset
        static_assert(std::is_base_of<Asset, T>::value, "Registered assets must derive from Asset class!");
        std::unique_ptr<Asset> upcastedAsset = std::move(asset);

        auto& assetsOfTypeT = assetDatas[std::type_index(typeid(T))];

        // Attempt to place the asset in the asset manager
        while(true) {
            auto it = assetsOfTypeT.find(upcastedAsset->getId());
            // No conflicting ID
            if(it == assetsOfTypeT.end()) {
                std::string id = upcastedAsset->getId();
                assetsOfTypeT[upcastedAsset->getId()] = AssetData();
                assetsOfTypeT[upcastedAsset->getId()].asset = std::move(upcastedAsset);

                // Restore the type from Asset to type T
                return AssetHandle<T>(id);
            
            // Must add a suffix to generate a unique ID
            } else {
                std::string newId;
                std::regex r(R"(^(.*_)(\d+)$)");
                std::smatch match;
                if(std::regex_search(upcastedAsset->getId(), match, r)) {
                    newId = match[1].str() + std::to_string(std::stoi(match[2].str())+1);
                } else {
                    newId = upcastedAsset->getId() + "_1";
                }
                upcastedAsset->setId(newId);
            }
        }
    }

    /**
     * @brief Gets an AssetHandle<T> by ID.
     * 
     * Returns AssetHandle<T>() on failure.
     */
    template<typename T>
    AssetHandle<T> getHandleById(const std::string& assetId) {
        auto& assetsOfTypeT = assetDatas[std::type_index(typeid(T))];
        auto it = assetsOfTypeT.find(assetId);

        if(it != assetsOfTypeT.end()) {
            // Restore the type from asset to type T
            return AssetHandle<T>(assetId);
        } else {
            return AssetHandle<T>();
        }
    }

    /**
     * @brief Returns true if an asset of ID and type T exist in the Asset Manager.
     */
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

    /**
     * @brief Returns a raw pointer to an asset if present.
     */
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

    /**
     * @brief Acquires the specified asset and all of its dependencies.
     */
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

    /**
     * @brief Releases the reference count on the asset and all of the asset's dependencies.
     */
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
};

} // namespace vkrt

#endif // VKRT_ASSETMANAGER_HPP