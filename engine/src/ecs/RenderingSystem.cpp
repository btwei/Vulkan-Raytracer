#include "RenderingSystem.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "CameraComponent.hpp"
#include "MeshComponent.hpp"
#include "TransformComponent.hpp"

namespace vkrt {
    
void RenderingSystem::update(std::vector<std::unique_ptr<Entity>>& entityList, GlobalSingletons globalSingletons) {
    // In the future, probably make a separate transform updating system
    
    // Get camera entity and update transform
    if(globalSingletons.activeCameraEntity != nullptr) {
        bool active = globalSingletons.activeCameraEntity->isActive();
        vkrt::CameraComponent* cameraComponent = globalSingletons.activeCameraEntity->getComponent<vkrt::CameraComponent>();
        vkrt::TransformComponent* transformComponent = globalSingletons.activeCameraEntity->getComponent<vkrt::TransformComponent>();

        if(active && cameraComponent && transformComponent) {
            // Rebuild View Matrix
            if(transformComponent->isDirty) {
                glm::mat4 T = glm::translate(glm::mat4(1.0f), transformComponent->position);
                glm::mat4 R = glm::mat4_cast(transformComponent->rotation);
                glm::mat4 S = glm::scale(glm::mat4(1.0f), transformComponent->scale);

                transformComponent->cachedTransform = T * R * S;

                // rebuild all dependent matrices
                cameraComponent->viewMatrix = glm::inverse(T * R);

                // mark clean
                transformComponent->isDirty = false;
                _renderer->setViewMatrix(cameraComponent->viewMatrix);
            }

            // Rebuild Projection Matrix
            if(cameraComponent->isDirty) {
                // rebuild projection matrix
                cameraComponent->isDirty = false;
                _renderer->setProjectionMatrix(cameraComponent->fov, cameraComponent->nearPlane, cameraComponent->farPlane);
            }
        }
    }

    // For every mesh component that is active and not acquired, acquire it
    // For every mesh component that is active and has a dirty transform, update the transform
    std::vector<vkrt::BlasInstance> instances;

    for(std::unique_ptr<Entity>& entityPtr : entityList) {
        bool active = entityPtr->isActive();
        MeshComponent* meshComponent = entityPtr->getComponent<MeshComponent>();
        TransformComponent* transformComponent = entityPtr->getComponent<TransformComponent>();

        if(active && meshComponent && transformComponent) {
            vkrt::BlasInstance blasInstance;
            // Acquire the asset handles. This guarantees that the assets are loaded on the gpu.
            if(!meshComponent->isAcquired) {
                _assetManager->acquireAsset(meshComponent->modelHandle);

                ModelAsset* modelAsset = _assetManager->getAsset<ModelAsset>(meshComponent->modelHandle.getId());
                AssetHandle<MeshAsset> meshHandle = modelAsset->getMeshHandle();
                MeshAsset* meshAsset = _assetManager->getAsset<MeshAsset>(meshHandle.getId());

                meshComponent->cachedInstance.blasAddress = meshAsset->_blasResources.blasAddress;
                meshComponent->cachedInstance.instanceIndex = 0;

                meshComponent->isAcquired = true;
            }

            if(transformComponent->isDirty) {
                glm::mat4 T = glm::translate(glm::mat4(1.0f), transformComponent->position);
                glm::mat4 R = glm::mat4_cast(transformComponent->rotation);
                glm::mat4 S = glm::scale(glm::mat4(1.0f), transformComponent->scale);

                transformComponent->cachedTransform = T * R * S;

                // rebuild all dependent matrices
                meshComponent->isDirty = true;

                // mark clean
                transformComponent->isDirty = false;
            }

            if(meshComponent->isDirty) {
                // Convert mat4 to Vulkan VkTransformMatrixKHR
                glm::mat4 transposedTransform = glm::transpose(transformComponent->cachedTransform);
                memcpy(&meshComponent->cachedInstance.transform.matrix, glm::value_ptr(transposedTransform), sizeof(float) * 12);
            }

            instances.push_back(meshComponent->cachedInstance);
        }
    }

    _renderer->setTLASBuild(std::move(instances));
}

} // namespace vkrt
