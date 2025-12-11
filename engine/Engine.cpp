#include "Engine.hpp"

namespace vkrt {

Engine::Engine() {

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
}

void Engine::run() {
    while(!_window->getShouldClose()) {
        _window->handleEvents();

        _renderer->submitFrame();
    }
}

void Engine::destroy() {
    _window->close();
}

} // namespace vkrt