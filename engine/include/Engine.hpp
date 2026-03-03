#ifndef VKRT_ENGINE_HPP
#define VKRT_ENGINE_HPP

#include <memory>
#include <string>

#include "AssetManager.hpp"
#include "EntityManager.hpp"
#include "InputManager.hpp"
#include "Renderer.hpp"
#include "Window.hpp"

namespace vkrt {

/**
 * @class Engine
 * @brief Top-Level class for the VKRT engine.
 * 
 * This is the top-level class for the Vulkan Raytracing engine. All interactions
 * with the engine are handled through this class. See member functions for more
 * details.
*/
class Engine {
public:
    Engine(int argc, char** argv);
    ~Engine();

    /**
     * @brief Initializes the VKRT engine.
     * 
     * This function must be called before using any other engine function. 
     * 
     * @param[in] windowName The name to display on the application created window.
     * @param[in] width The initial width of the window.
     * @param[in] height The initial height of the window.
     */
    void init(const std::string& windowName, int width, int height);

    /**
     * @brief Runs the VKRT engine.
     * 
     * This function will occupy the main loop and run until the program is quit.
     */
    void run();

    /**
     * @brief Destroys the VKRT engine.
     * 
     * This function is called automatically when the engine goes out of scope.
     */
    void destroy();

    /**
     * @brief Gets a pointer to the asset manager.
     * 
     * The asset manager is used to import asset files into asset handles. See 
     * IAssetManager for more details.
     * 
     * @return Returns a pointer to the asset manager.
     */
    IAssetManager* getAssetManager() { return _assetManager.get(); }

    /**
     * @brief Gets a pointer to the entity manager.
     * 
     * The entity manager is used to create new entities, attach components to
     * existing entities, and to register user defined systems. See IEntityManager
     * for more details.
     * 
     * @return Returns a pointer to the entity manager.
     */
    IEntityManager* getEntityManager() { return _entityManager.get(); }

private:
    int _argc;
    char** _argv;

    bool _isInitialized = false;
    bool _isRunning = false;

    std::unique_ptr<Window> _window;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<AssetManager> _assetManager;
    std::unique_ptr<InputManager> _inputManager;
    std::unique_ptr<EntityManager> _entityManager;
};

} // namespace vkrt

#endif // VKRT_ENGINE_HPP