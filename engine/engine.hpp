#ifndef VKRT_ENGINE_HPP
#define VKRT_ENGINE_HPP

#include "Renderer.hpp"

namespace vkrt {

class Engine {
public:
    Engine();
    ~Engine();

    void init();
    void run();
    void cleanup();
private:

};

} // namespace vkrt

#endif // VKRT_ENGINE_HPP