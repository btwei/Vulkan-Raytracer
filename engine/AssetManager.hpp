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
 * class is inherited from to implement each asset type.
 */
class Asset {
public:
    Asset(const std::string& assetId) :_assetId(assetId) {}
    virtual ~Asset() = default;

    const std::string& getId() const { return _assetId; }
    bool isLoaded() const { return _loaded; }

    bool load() { _loaded = doLoad(); return _loaded; }
    bool unload() { doUnload(); _loaded = false; }

protected:
    std::string _assetId;
    bool _loaded;

    virtual bool doLoad() = 0;
    virtual bool doUnload() = 0;
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

class Mesh : Asset {

};

class Material : Asset {

};

struct ImportResult {
    bool success = false;
};

class AssetManager {
public:
    AssetManager(Renderer* renderer);

    void init();
    bool loadAsset(const std::filesystem::path& filepath);

private:
    Renderer* _renderer;

    std::unordered_map<std::type_index, std::unordered_map<std::string, std::shared_ptr<Asset>>> assets;

    struct AssetData {
        std::shared_ptr<Asset> asset;
        int refCount;
    };

    std::unordered_map<std::type_index, std::unordered_map<std::string, Asset>> refCounts;

	AllocatedImage _whiteImage;
	AllocatedImage _blackImage;
	AllocatedImage _greyImage;
	AllocatedImage _errorCheckerboardImage;

    ImportResult loadGLTF(const std::filesystem::path& filepath);
};

} // namespace vkrt

#endif // VKRT_ASSETMANAGER_HPP