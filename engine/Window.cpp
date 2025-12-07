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

    _window = SDL_CreateWindow(windowName.c_str(), width, height, SDL_WINDOW_VULKAN);
    if(_window == NULL) throw std::runtime_error("Failed to create SDL3 Window!");

    _windowID = SDL_GetWindowID(_window);
    SDL_GetWindowSize(_window, &_width, &_height);
    SDL_GetWindowSizeInPixels(_window, &_framebufferWidth, &_framebufferHeight);
}

void Window::close() {
    SDL_DestroyWindow(_window);
    _window = nullptr;
}

bool Window::handleEvents() {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_EVENT_QUIT) _shouldClose = true;

        // TODO: handle events and update Window class's internal state
        // will do this later as I determine what states are needed / how to handle dearimgui integration--specifically capturing inputs
    }

    return true;
}

}