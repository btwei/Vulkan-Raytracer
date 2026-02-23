#include "Window.hpp"

#include <cassert>
#include <stdexcept>

namespace vkrt {

int Window::_instanceCount = 0;

Window::Window() {
    if(_instanceCount == 0) SDL_Init(SDL_INIT_VIDEO);
    _instanceCount++;
}

Window::Window(const std::string& windowName, int width, int height) {
    if(_instanceCount == 0) SDL_Init(SDL_INIT_VIDEO);
    _instanceCount++;
    open(windowName, width, height);
}

Window::~Window() {
    if(_window) close();
    _instanceCount--;
    if(_instanceCount == 0) SDL_Quit();
}

void Window::open(const std::string& windowName, int width, int height) {
    if(_window != nullptr) throw std::runtime_error("Each window object may only open one window!");

    _window = SDL_CreateWindow(windowName.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if(_window == NULL) throw std::runtime_error("Failed to create SDL3 Window!");

    _windowID = SDL_GetWindowID(_window);
    SDL_GetWindowSize(_window, &_width, &_height);
    SDL_GetWindowSizeInPixels(_window, &_framebufferWidth, &_framebufferHeight);
}

void Window::close() {
    SDL_DestroyWindow(_window);
    _window = nullptr;
}

VkSurfaceKHR Window::createSurface(VkInstance instance) {
    VkSurfaceKHR surface;
    bool result = SDL_Vulkan_CreateSurface(_window, instance, nullptr, &surface);
    if(!result) throw std::runtime_error("Failed to create Vulkan surface!");
    return surface;
}

void Window::registerEventHandler(std::function<void(SDL_Event e)> eventHandler) {
    _eventHandlerCallbacks.push_back(eventHandler);
}

bool Window::handleEvents() {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        // Manage window state
        if(e.type == SDL_EVENT_QUIT) _shouldClose = true;
        if(e.type == SDL_EVENT_WINDOW_RESIZED) {
            {
                std::lock_guard<std::mutex> guard(mutex);
                SDL_GetWindowSize(_window, &_width, &_height);
                SDL_GetWindowSizeInPixels(_window, &_framebufferWidth, &_framebufferHeight);
                _wasResized = true;
            }
        }

        // Handle engine registered callbacks
        // note: This method was chosen for easy integration with dearImGui
        for(std::function<void(SDL_Event e)>& eventHandler : _eventHandlerCallbacks) {
            eventHandler(e);
        }
    }

    return true;
}

bool Window::getWasResized() {
    std::lock_guard<std::mutex> guard(mutex);
    bool resized = _wasResized;
    if(resized == true) _wasResized = false;
    return resized;
}

std::string Window::getBinaryPath() const {
    // Copies the const char* into a std::string and returns
    return SDL_GetBasePath();
}

}