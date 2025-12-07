#ifndef VKRT_WINDOW_HPP
#define VKRT_WINDOW_HPP

#include <string>

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

namespace vkrt {

/**
 * @class Window
 * @brief Handles windowing via SDL3 using RAII principles
 * 
 * @note Window functions should be called from the main thread!
 */
class Window {
public:
    Window();
    Window(const std::string& windowName, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void open(const std::string& windowName, int width, int height);
    void close();

    bool handleEvents();
    bool getShouldClose() const { return _shouldClose; };

    SDL_Window* getWindow() const { return _window; };
    int getWidth() const { return _width; };
    int getHeight() const { return _height; };
    int getFramebufferWidth() const { _framebufferWidth; };
    int getFramebufferHeight() const { _framebufferHeight; };

private:
    static int _instanceCount;

    SDL_Window* _window = nullptr;
    SDL_WindowID _windowID = 0;
    int _width = 0;
    int _height = 0;
    int _framebufferWidth = 0;
    int _framebufferHeight = 0;

    bool _shouldClose = false;
};

} // namespace vkrt

#endif // VKRT_WINDOW_HPP