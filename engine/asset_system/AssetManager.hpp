#ifndef VKRT_ASSETMANAGER_HPP
#define VKRT_ASSETMANAGER_HPP

#include <filesystem>
#include <regex>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "Renderer.hpp"
#include "VulkanTypes.hpp"

namespace vkrt {

/**
 * @class Asset
 * @brief Assets represent any external file that is loaded into an AssetManager class. This
 * class is inherited from to implement each asset type. AssetIds are strings, which mean that these
 * need to be unique names per asset type.
 * 
 */
class Asset {
public:
    Asset(const std::string& assetId) :_assetId(assetId) { }
    virtual ~Asset() { unload(); }

    const std::string& getId() const { return _assetId; }
    void setId(const std::string& assetId) { _assetId = assetId; }
    bool isLoaded() const { return _loaded; }

    bool load() { _loaded = doLoad(); return _loaded; }
    bool unload() { if(doUnload()) _loaded = false; return _loaded; }
    virtual bool onRef() { return true; }
    virtual bool onUnref() { return true; }

protected:
    virtual bool doLoad() { return true; }
    virtual bool doUnload() { return true; }

private:
    std::string _assetId;
    bool _loaded;
};

template<typename T>
class AssetHandle;

class ModelAsset;
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
            return AssetHandle<T>(assetId, this);
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
        if(it != assetsOfTypeT.end() && it->second.asset->onRef()) {
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

        if(it != refCountsForTypeT.end() && it->second.asset->onUnref()) {
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
                return AssetHandle<T>(id, this);
            
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

/**
 * @class AssetHandle
 * @brief AssetHandles are the user facing handles for Assets which are stored in the AssetManager
 */
template<typename T>
class AssetHandle {
public:
    AssetHandle() : _assetManager(nullptr) {};
    AssetHandle(const std::string& assetId, AssetManager* assetManager) : _assetId(assetId), _assetManager(assetManager) {};

    T* get() { return (_assetManager == nullptr) ? _assetManager->getAsset<T>(_assetId) : nullptr; }
    const std::string& getId() { return _assetId; }
    bool isValid() { return _assetManager && _assetManager->hasAsset<T>(_assetId); }

private:
    std::string _assetId;
    AssetManager* _assetManager;
};

} // namespace vkrt

#endif // VKRT_ASSETMANAGER_HPP