#ifndef VKRT_ASSETHANDLE_HPP
#define VKRT_ASSETHANDLE_HPP

#include <memory>
#include <string>

namespace vkrt {

class AssetManager;

/**
 * @class AssetHandle
 * @brief AssetHandles are the user facing handles for Assets which are stored in the AssetManager.
 * 
 * These are primarily created through use of the importAsset function in the AssetManager.
 */
template<typename T>
class AssetHandle {
public:
    AssetHandle() {};
    AssetHandle(const AssetHandle& other) : _assetId(other._assetId) {};

    const std::string& getId() { return _assetId; }

    friend class AssetManager; // Factory class for Asset Handles
private:
    std::string _assetId;

    AssetHandle(const std::string& assetId) : _assetId(assetId) {};
};

} // namespace vkrt

#endif // VKRT_ASSETHANDLE_HPP