#ifndef VKRT_ASSETMANAGER_HPP
#define VKRT_ASSETMANAGER_HPP

#include <filesystem>
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
 */
class Asset {
public:
    Asset(const std::string& assetId) :_assetId(assetId) { }
    virtual ~Asset() { unload(); }

    const std::string& getId() const { return _assetId; }
    void setId(const std::string& assetId) { _assetId = assetId; }
    bool isLoaded() const { return _loaded; }

    bool load() { _loaded = doLoad(); return _loaded; }
    bool unload() { doUnload(); _loaded = false; }
    virtual bool onRef() = 0;
    virtual bool onUnref() = 0;

protected:
    virtual bool doLoad() = 0;
    virtual bool doUnload() = 0;

private:
    std::string _assetId;
    bool _loaded;
};

/**
 * @class AssetHandle
 * @brief AssetHandles are the user facing handles for Assets which are stored in the AssetManager
 */
template<typename T>
class AssetHandle {
public:
    AssetHandle() : _assetManager(nullptr) {};
    AssetHandle(const std::string& assetId, AssetManager* assetManager) : _assetId(assetId) _assetManager(assetManager) {};

    T* get() { return _assetManager : _assetManager->getAsset<T>(_assetId) ? nullptr; }
    const std::string& getId() { return _assetId; }
    bool isValid() { return _assetManager && _assetManager->hasAsset<T>(_assetId); }

private:
    std::string _assetId;
    AssetManager* _assetManager;
};

struct ImportResult {
    bool success = false;
};

class AssetManager {
public:
    AssetManager(Renderer* renderer);

    void init();
    ImportResult importAsset(const std::filesystem::path& filepath);

    template<typename T>
    AssetHandle<T> fetchHandleById(const std::string& assetId);

    template<typename T>
    bool acquireAsset(AssetHandle<T> assetHandle);

    template<typename T>
    bool releaseAsset(AssetHandle<T> assetHandle);

    template<typename T>
    AssetHandle<T> registerAsset(std::shared_ptr<T> asset);

private:
    struct AssetData {
        std::shared_ptr<Asset> asset;
        int refCount;
    };

    Renderer* _renderer;

    std::unordered_map<std::type_index, std::unordered_map<std::string, std::shared_ptr<AssetData>>> assetDatas;

    ImportResult loadGLTF(const std::filesystem::path& filepath);

};

} // namespace vkrt

#endif // VKRT_ASSETMANAGER_HPP