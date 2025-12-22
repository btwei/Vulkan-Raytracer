#include "App.hpp"

#include <filesystem>

App::App(int argc, char* argv[]) :
    _argc(argc),
    _argv(argv),
    _engine(argc, argv){ }

void App::run() {
    _engine.init("Vulkan Raytracer", 800, 600);

    // TODO: Add application logic here-- cameras, object loading, etc.
    _engine.loadAsset("../assets/horse_statue_01_8k.gltf");

    _engine.run();
}