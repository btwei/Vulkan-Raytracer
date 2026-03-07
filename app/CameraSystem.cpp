#include "CameraSystem.hpp"

#include "CameraComponent.hpp"
#include "TransformComponent.hpp"

void CameraSystem::update(std::vector<std::unique_ptr<vkrt::Entity>>& entityList, vkrt::GlobalSingletons globalSingletons) {
    // If no active camera, then set the first one found to active
    if(globalSingletons.activeCameraEntity == nullptr) {
        for(std::unique_ptr<vkrt::Entity>& entity : entityList) {
            vkrt::CameraComponent* cameraComponent = entity->getComponent<vkrt::CameraComponent>();

            if(cameraComponent) {
                globalSingletons.activeCameraEntity = entity.get();
                break;
            }
        }
    }
    
    // Use WASD to move the camera transform
    if(globalSingletons.activeCameraEntity != nullptr) {
        bool active = globalSingletons.activeCameraEntity->isActive();
        vkrt::CameraComponent* cameraComponent = globalSingletons.activeCameraEntity->getComponent<vkrt::CameraComponent>();
        vkrt::TransformComponent* transformComponent = globalSingletons.activeCameraEntity->getComponent<vkrt::TransformComponent>();

        if(active && cameraComponent && transformComponent) {
            // Use input state to move the camera entity's transform 
            if(globalSingletons.inputState.keyboardState.keys[vkrt::VKRT_W_KEY].down) {
                transformComponent->position += glm::vec3(0.0, 0.0, 0.01);
                cameraComponent->isDirty = true;
            }

            if(globalSingletons.inputState.keyboardState.keys[vkrt::VKRT_S_KEY].down) {
                transformComponent->position -= glm::vec3(0.0, 0.0, 0.01);
                cameraComponent->isDirty = true;
            }

            if(globalSingletons.inputState.keyboardState.keys[vkrt::VKRT_A_KEY].down) {
                transformComponent->position -= glm::vec3(0.01, 0.0, 0.0);
                cameraComponent->isDirty = true;
            }

            if(globalSingletons.inputState.keyboardState.keys[vkrt::VKRT_D_KEY].down) {
                transformComponent->position += glm::vec3(0.01, 0.0, 0.01);
                cameraComponent->isDirty = true;
            }
        }
    }
}