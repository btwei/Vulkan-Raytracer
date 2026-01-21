#ifndef VKRT_ENGINE_HPP
#define VKRT_ENGINE_HPP

#include <memory>
#include <string>

#include "AssetManager.hpp"
#include "EntityManager.hpp"
#include "Renderer.hpp"
#include "Window.hpp"

namespace vkrt {

class Engine {
public:
    Engine(int argc, char** argv);
    ~Engine();

    void init(const std::string& windowName, int width, int height);
    void run();
    void destroy();

    AssetManager* getAssetManager() { return _assetManager.get(); }
    EntityManager* getEntityManager() { return _entityManager.get(); }

private:
    int _argc;
    char** _argv;

    bool isInitialized = false;
    bool isRunning = false;

    std::unique_ptr<Window> _window;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<AssetManager> _assetManager;
    std::unique_ptr<EntityManager> _entityManager;
};

} // namespace vkrt

#endif // VKRT_ENGINE_HPP