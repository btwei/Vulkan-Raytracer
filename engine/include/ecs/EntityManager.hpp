#ifndef VKRT_ENTITYMANAGER_HPP
#define VKRT_ENTITYMANAGER_HPP

#include "AssetManager.hpp"
#include "CameraComponent.hpp"
#include "Entity.hpp"
#include "MeshComponent.hpp"
#include "TransformComponent.hpp"
#include "Renderer.hpp"

namespace vkrt {
    
class EntityManager {
public:

    EntityManager(Renderer* renderer, AssetManager* assetManager)
    : _renderer(renderer)
    , _assetManager(assetManager) { };

    ~EntityManager();

    void init();
    void update();

    Entity* createNewEntity(const std::string entityName);
    bool removeEntity(const std::string entityName);

private:
    Renderer* _renderer;
    AssetManager* _assetManager;

    std::vector<std::unique_ptr<Entity>> _entityList;
};

} // namespace vkrt

#endif // VKRT_ENTITYMANAGER_HPP