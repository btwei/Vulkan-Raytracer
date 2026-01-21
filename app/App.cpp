#include "App.hpp"

#include <filesystem>

App::App(int argc, char* argv[]) :
    _argc(argc),
    _argv(argv),
    _engine(argc, argv){ }

void App::run() {
    _engine.init("Vulkan Raytracer", 800, 600);

    // TODO: Add application logic here-- cameras, object loading, etc.
    vkrt::ImportResult statueImport = _engine.getAssetManager()->importAsset("../assets/horse_statue_01_8k/horse_statue_01_8k.gltf");
    vkrt::AssetHandle<vkrt::ModelAsset> statueHandle = statueImport.modelHandles[0];

    vkrt::Entity* statue = _engine.getEntityManager()->createNewEntity("statueEntity");
    statue->addComponent<vkrt::MeshComponent>(statueHandle);
    statue->addComponent<vkrt::TransformComponent>();
    
    vkrt::Entity* camera = _engine.getEntityManager()->createNewEntity("cameraEntity");
    camera->addComponent<vkrt::CameraComponent>();
    vkrt::TransformComponent* cameraTransform = camera->addComponent<vkrt::TransformComponent>();
    cameraTransform->position = glm::vec3(0.0f, 0.0f, 10.0f);

    _engine.run();
}