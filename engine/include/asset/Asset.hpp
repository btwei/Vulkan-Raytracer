#ifndef VKRT_ASSET_HPP
#define VKRT_ASSET_HPP

#include <string>
#include <typeindex>
#include <vector>

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
    std::vector<std::pair<std::type_index, std::string>> getReferenceList() const { return _referenceList; }

    bool load() { _loaded = doLoad(); return _loaded; }
    bool unload() { if(doUnload()) _loaded = false; return _loaded; }

protected:
    std::vector<std::pair<std::type_index, std::string>> _referenceList;

    virtual bool doLoad() { return true; }
    virtual bool doUnload() { return true; }

private:
    std::string _assetId;
    bool _loaded;
};

} // namespace vkrt

#endif // VKRT_ASSET_HPP