#ifndef VKRT_MESHCOMPONENT_HPP
#define VKRT_MESHCOMPONENT_HPP

#include "AssetHandle.hpp"
#include "Component.hpp"
#include "MaterialAsset.hpp"
#include "MeshAsset.hpp"
#include "ModelAsset.hpp"

namespace vkrt {

/**
 * @class MeshComponent
 * @brief A Component representing a mesh and its materials
 * 
 * Each mesh may have multiple materials associated with it. This
 * is because a mesh can have sections or 'submeshes' that are supposed
 * to be rendered differently.
 * 
 * The importer class will provide ModelAsset handles as an easy way to
 * preserve the mesh and material connections from imported files. These 
 * handles can be used to construct MeshComponents directly. 
 */
class MeshComponent : public Component {
public:
    MeshComponent(AssetHandle<ModelAsset> modelHandle) {
        if(modelHandle.isValid()) {
            std::shared_ptr<ModelAsset> ptr = modelHandle.get();
            meshHandle = ptr->getMeshHandle();
            materialHandles = ptr->getMaterials();
        }
    }

    AssetHandle<MeshAsset> meshHandle;
    std::vector<AssetHandle<MaterialAsset>> materialHandles;
    
    /**
     * @brief tracks the acquired/released status of the handles in the assetManager
     */
    bool isAcquired = false;
};

} // namespace vkrt

#endif // VKRT_MESHCOMPONENT_HPP