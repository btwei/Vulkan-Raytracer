#include "TestSystem.hpp"

#include <iostream>

TestSystem::TestSystem() { }

TestSystem::~TestSystem() { }

void TestSystem::update(std::vector<std::unique_ptr<vkrt::Entity>>& entityList, const vkrt::GlobalSingletons globalSingletons) {
    if(globalSingletons.inputState.keyboardState.keys[vkrt::VKRT_W_KEY].down) {
        printf("W key pressed!\n");
    }
}