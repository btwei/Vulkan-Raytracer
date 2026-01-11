#ifndef VKRT_ASSET_HPP
#define VKRT_ASSET_HPP

#include <string>

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

} // namespace vkrt

#endif // VKRT_ASSET_HPP