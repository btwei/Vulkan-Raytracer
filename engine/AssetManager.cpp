#include "AssetManager.hpp"

namespace vkrt {

ImportResult AssetManager::loadAsset(const std::filesystem::path&& filepath) {
    if(filepath.) {
        return loadGLTF(filepath);
    }

    return false;
}

template<typename T>
T* AssetHandle<T>::get() {

}

template<typename T>
const std::string& AssetHandle<T>::getId() {

}

template<typename T>
bool AssetHandle<T>::isValid() {

}

}