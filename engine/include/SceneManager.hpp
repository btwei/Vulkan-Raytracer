#ifndef VKRT_SCENEMANAGER_HPP
#define VKRT_SCENEMANAGER_HPP

#include <memory>
#include <vector>

#include "Entity.hpp"

namespace vkrt {

class Scene {
public:

    void getDiff();

private:

};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();
    
    Scene* getActiveScene();

private:
    std::vector<std::unique_ptr<Scene>> _scenes;

};

}

#endif // VKRT_SCENEMANAGER_HPP