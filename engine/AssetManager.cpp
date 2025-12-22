#include "AssetManager.hpp"

namespace vkrt {

bool AssetManager::loadAsset(const std::filesystem::path&& filepath) {
    if(filepath.) {
        return loadGLTF(filepath);
    }

    return false;
}

}