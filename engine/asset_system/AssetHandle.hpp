#ifndef VKRT_ASSETHANDLE_HPP
#define VKRT_ASSETHANDLE_HPP

#include <memory>
#include <string>

namespace vkrt {

/**
 * @class AssetHandle
 * @brief AssetHandles are the user facing handles for Assets which are stored in the AssetManager
 */
template<typename T>
class AssetHandle {
private:
    std::string _assetId;
    std::weak_ptr<T> _assetPtr;
public:
    AssetHandle() {};
    AssetHandle(const std::string& assetId, std::weak_ptr<T> assetPointer) : _assetId(assetId), _assetPtr(assetPointer) {};

    std::shared_ptr<T> get() { return _assetPtr.lock(); }
    const std::string& getId() { return _assetId; }
    bool isValid() { return _assetPtr.expired(); }
};

} // namespace vkrt

#endif // VKRT_ASSETHANDLE_HPP