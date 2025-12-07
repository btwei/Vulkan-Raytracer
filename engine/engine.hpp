#ifndef VKRT_ENGINE_HPP
#define VKRT_ENGINE_HPP

#include <memory>
#include <string>

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
};

} // namespace vkrt

#endif // VKRT_ENGINE_HPP