#include "ModelAsset.hpp"

namespace vkrt {

ModelAsset::ModelAsset(const std::string& assetId, ModelInfo modelInfo)
    : Asset(assetId)
    , _mesh(modelInfo.mesh)
    , _materials(modelInfo.materials) { 
        _referenceList.push_back({std::type_index(std::type_index(typeid(MeshAsset))), modelInfo.mesh.getId()});
        for(AssetHandle<MaterialAsset>& handle : modelInfo.materials)
            _referenceList.push_back({std::type_index(std::type_index(typeid(MaterialAsset))), handle.getId()});
    }

ModelAsset::~ModelAsset() { }

}