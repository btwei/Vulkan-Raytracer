#include "App.hpp"

#include <filesystem>

#include "CameraSystem.hpp"
#include "TestSystem.hpp"

App::App(int argc, char* argv[]) :
    _argc(argc),
    _argv(argv),
    _engine(argc, argv){ }

void App::run() {
    _engine.init("Vulkan Raytracer", 800, 600);

    // Import a GLTF file into handles
    vkrt::ImportResult statueImport = _engine.getAssetManager()->importAsset("../assets/horse_statue_01_8k/horse_statue_01_8k.gltf");
    vkrt::AssetHandle<vkrt::ModelAsset> statueHandle = statueImport.modelHandles[0];

    // Create a new statue entity
    vkrt::Entity* statue = _engine.getEntityManager()->createNewEntity("statueEntity");
    statue->addComponent<vkrt::MeshComponent>(statueHandle);
    statue->addComponent<vkrt::TransformComponent>();
    
    // Create a camera entity
    vkrt::Entity* camera = _engine.getEntityManager()->createNewEntity("cameraEntity");
    camera->addComponent<vkrt::CameraComponent>();
    vkrt::TransformComponent* cameraTransform = camera->addComponent<vkrt::TransformComponent>();
    cameraTransform->position = glm::vec3(0.0f, 0.0f, 10.0f);

    // Register systems in order
    std::shared_ptr<vkrt::System> testSystem = std::make_shared<TestSystem>();
    _engine.getEntityManager()->registerSystem(testSystem);

    std::shared_ptr<vkrt::System> cameraSystem = std::make_shared<CameraSystem>();
    _engine.getEntityManager()->registerSystem(cameraSystem);

    // Run
    _engine.run();
}