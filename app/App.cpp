#include "App.hpp"

App::App(int argc, char* argv[]) :
    _argc(argc),
    _argv(argv){ }

void App::run() {
    _engine.init("Vulkan Raytracer", 800, 600);

    // TODO: Add application logic here-- cameras, object loading, etc.

    _engine.run();
}