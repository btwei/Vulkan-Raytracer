#ifndef VKRT_ENGINE_HPP
#define VKRT_ENGINE_HPP

#include <memory>
#include <string>

#include "AssetManager.hpp"
#include "Entity.hpp"
#include "ModelLoader.hpp"
#include "Renderer.hpp"
#include "SceneManager.hpp"
#include "Window.hpp"

namespace vkrt {

class Engine {
public:
    Engine(int argc, char** argv);
    ~Engine();

    void init(const std::string& windowName, int width, int height);
    void run();
    void destroy();

    /**
     * @brief loadAsset(const std::string& filepath)
     * 
     * @param filepath This should be a filepath relative to the binary for best practice and portability
     */
    void loadAsset(const std::filesystem::path& filepath);

private:
    int _argc;
    char** _argv;

    bool isInitialized = false;
    bool isRunning = false;

    std::unique_ptr<Window> _window;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<ModelLoader> _modelLoader;
    std::unique_ptr<SceneManager> _sceneManager;
    std::unique_ptr<AssetManager> _assetManager;

    std::vector<std::unique_ptr<Entity>> _entityList;
};

} // namespace vkrt

#endif // VKRT_ENGINE_HPP