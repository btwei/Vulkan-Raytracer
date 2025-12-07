#ifndef VKRT_RENDERER_HPP
#define VKRT_RENDERER_HPP

#include "Window.hpp"

namespace vkrt {

class Renderer {
public:
    Renderer(Window* window);
    ~Renderer();

    void init();
    void drawFrame();
    
private:

};

} // namespace vkrt

#endif // VKRT_RENDERER_HPP