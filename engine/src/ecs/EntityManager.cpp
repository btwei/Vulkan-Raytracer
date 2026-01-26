#include "EntityManager.hpp"

namespace vkrt {
    
EntityManager::~EntityManager() {

}

void EntityManager::init() {

}

void EntityManager::update() {
    for(std::unique_ptr<Entity>& entity : _entityList) {
        renderingSystem.update(entity.get());
    }
}

Entity* EntityManager::createNewEntity(const std::string entityName) {
    std::unique_ptr newEntity = std::make_unique<Entity>(entityName);
    Entity* entityPtr = newEntity.get();
    _entityList.push_back(std::move(newEntity));

    return entityPtr;
}

bool EntityManager::removeEntity(const std::string entityName) {
    for(auto it = _entityList.begin(); it != _entityList.end(); ++it) {
        if(it->get()->getName() == entityName) {
            _entityList.erase(it);
            return true;
        }
    }
    return false;
}

} // namespace vkrt
