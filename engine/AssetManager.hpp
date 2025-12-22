#ifndef VKRT_ASSETMANAGER_HPP
#define VRTT_ASSETMANAGER_HPP

#include <filesystem>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "Renderer.hpp"
#include "VulkanTypes.hpp"

namespace vkrt {

enum AssetState{
    UNINITIALIZED,
    CPU_LOADED,
    GPU_LOADED,
    ACTIVE
};

class Asset {
protected:
    std::string assetName;


public:
    virtual ~Asset() = default;

};

template<typename T>
class AssetHandle {

};

class Mesh : Asset {

};

class Material : Asset {

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

    bool loadGLTF(const std::filesystem::path& filepath);
};

} // namespace vkrt

#endif // VKRT_ASSETMANAGER_HPP