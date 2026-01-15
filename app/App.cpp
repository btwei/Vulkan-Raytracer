#include "App.hpp"

#include <filesystem>

App::App(int argc, char* argv[]) :
    _argc(argc),
    _argv(argv),
    _engine(argc, argv){ }

void App::run() {
    _engine.init("Vulkan Raytracer", 800, 600);

    // TODO: Add application logic here-- cameras, object loading, etc.
    vkrt::ImportResult horse = _engine.getAssetManager()->importAsset("../assets/horse_statue_01_8k/horse_statue_01_8k.gltf");
    vkrt::AssetHandle<vkrt::ModelAsset> handle = horse.modelHandles[0];

    _engine.getAssetManager()->acquireAsset(handle);

    _engine.run();
}