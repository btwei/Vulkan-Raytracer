#ifndef VKRT_WINDOW_HPP
#define VKRT_WINDOW_HPP

#include <mutex>
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
private:
    std::mutex mutex;

public:
    Window();
    Window(const std::string& windowName, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void open(const std::string& windowName, int width, int height);
    void close();

    VkSurfaceKHR createSurface(VkInstance instance);

    bool handleEvents();
    bool getShouldClose() const { return _shouldClose; };

    /**
     * @brief Gets _wasResized and expects caller to handle window resize. Sets _wasResized to false after calling.
     * 
     * @warning This function should only be called from the Renderer class. 
     */
    bool getWasResized();

    SDL_Window* getWindow() const { return _window; };
    int getWidth() { std::lock_guard<std::mutex> guard(mutex); return _width; };
    int getHeight() { std::lock_guard<std::mutex> guard(mutex); return _height; };
    int getFramebufferWidth() { std::lock_guard<std::mutex> guard(mutex); return _framebufferWidth; };
    int getFramebufferHeight() { std::lock_guard<std::mutex> guard(mutex); return _framebufferHeight; };

private:
    static int _instanceCount;

    SDL_Window* _window = nullptr;
    SDL_WindowID _windowID = 0;
    int _width = 0;
    int _height = 0;
    int _framebufferWidth = 0;
    int _framebufferHeight = 0;

    bool _shouldClose = false;
    bool _wasResized = false;
};

} // namespace vkrt

#endif // VKRT_WINDOW_HPP