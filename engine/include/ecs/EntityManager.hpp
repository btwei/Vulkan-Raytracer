#ifndef VKRT_ENTITYMANAGER_HPP
#define VKRT_ENTITYMANAGER_HPP

#include "AssetManager.hpp"
#include "CameraComponent.hpp"
#include "Entity.hpp"
#include "GlobalSingletons.hpp"
#include "InputManager.hpp"
#include "MeshComponent.hpp"
#include "RenderingSystem.hpp"
#include "TransformComponent.hpp"
#include "Renderer.hpp"

namespace vkrt {

/**
 * @class IEntityManager
 * @brief This is the user-facing ECS manager class
 */
class IEntityManager {
public:
    virtual ~IEntityManager() = default;

    /**
     * @brief Create a new Entity inside the ECS manager
     * @return A pointer to the newly created Entity
     */
    virtual Entity* createNewEntity(const std::string entityName) = 0;

    /**
     * @brief Removes the entity by name from the ECS manager
     */
    virtual bool removeEntity(const std::string entityName) = 0;

    /**
     * @brief Registers the provided System in the ECS manager. Systems act in the order registered.
     */
    virtual void registerSystem(std::shared_ptr<System> system) = 0;
};

/**
 * @class EntityManager
 * @brief This is the engine internal ECS manager class.
 */
class EntityManager final : public IEntityManager {
public:
    EntityManager(Renderer* renderer, AssetManager* assetManager, InputManager* inputManager)
    : _renderer(renderer)
    , _assetManager(assetManager)
    , _inputManager(inputManager)
    , _renderingSystem(assetManager, renderer) { };
    ~EntityManager();

    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;

    /**
     * @brief Initializes engine owned systems
     */
    void init();

    /**
     * @brief Calls system functions in order. Calls engine-owned renderer system last.
     */
    void update();

    Entity* createNewEntity(const std::string entityName) override;
    bool removeEntity(const std::string entityName) override;

    void registerSystem(std::shared_ptr<System> system) override;

private:
    Renderer* _renderer;
    AssetManager* _assetManager;
    InputManager* _inputManager;

    GlobalSingletons _globalSingletons;

    // Engine controlled systems
    RenderingSystem _renderingSystem;

    std::vector<std::unique_ptr<Entity>> _entityList;
    std::vector<std::shared_ptr<System>> _systemList;
};

} // namespace vkrt

#endif // VKRT_ENTITYMANAGER_HPP