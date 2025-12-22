#include "Engine.hpp"

namespace vkrt {

Engine::Engine(int argc, char** argv) : _argc(argc), _argv(argv) {

}

/**
 * @note Be aware that exceptions may be thrown -- using RAII/destructors is the best practice here
 */
Engine::~Engine() {
    destroy();
}

void Engine::init(const std::string& windowName, int width, int height) {
    _window = std::make_unique<Window>();
    _window->open(windowName, width, height);

    _renderer = std::make_unique<Renderer>(_window.get());
    _renderer->init();

    _assetManager = std::make_unique<AssetManager>(_renderer.get());

    _modelLoader = std::make_unique<ModelLoader>(_renderer.get());
}

void Engine::run() {
    while(!_window->getShouldClose()) {
        _window->handleEvents();

        _renderer->renderScene();
    }
}

void Engine::destroy() {
    _window->close();
}

void Engine::loadAsset(const std::filesystem::path& filepath) {
    _assetManager->loadAsset(filepath);
}

} // namespace vkrt