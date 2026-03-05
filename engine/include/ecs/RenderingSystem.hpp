#ifndef VKRT_RENDERINGSYSTEM_HPP
#define VKRT_RENDERINGSYSTEM_HPP

#include "AssetManager.hpp"
#include "Renderer.hpp"
#include "System.hpp"

namespace vkrt {

/**
 * @class RenderingSystem
 * @brief RenderingSystem is an engine-owned system that operates last in the ECS Manager. This
 * class is responsible for updating active camera view matrices and pushing the scene state to the
 * renderer.
 */
class RenderingSystem : public System {
public:
    RenderingSystem(AssetManager* assetManager, Renderer* renderer) : _assetManager(assetManager), _renderer(renderer) {}

    virtual void update(std::vector<std::unique_ptr<Entity>>& entityList, GlobalSingletons GlobalSingletons) override;

private:
    AssetManager* _assetManager;
    Renderer* _renderer;
};

} // namespace vkrt

#endif // VKRT_RENDERINGSYSTEM_HPP