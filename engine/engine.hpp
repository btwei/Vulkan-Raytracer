#ifndef VKRT_ENGINE_HPP
#define VKRT_ENGINE_HPP

#include <memory>
#include <string>

#include "ModelLoader.hpp"
#include "Renderer.hpp"
#include "Window.hpp"

namespace vkrt {

class Engine {
public:
    Engine();
    ~Engine();

    void init(const std::string& windowName, int width, int height);
    void run();
    void destroy();

private:
    bool isInitialized = false;
    bool isRunning = false;

    std::unique_ptr<Window> _window;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<ModelLoader> _modelLoader;
};

} // namespace vkrt

#endif // VKRT_ENGINE_HPP