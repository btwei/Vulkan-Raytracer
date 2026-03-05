#include "TestSystem.hpp"

#include <iostream>

void TestSystem::update(std::vector<std::unique_ptr<vkrt::Entity>>& entityList, vkrt::GlobalSingletons globalSingletons) {
    if(globalSingletons.inputState.keyboardState.keys[vkrt::VKRT_W_KEY].down) {
        printf("W key pressed!\n");
    }
    if(globalSingletons.inputState.keyboardState.keys[vkrt::VKRT_A_KEY].down) {
        printf("A key pressed!\n");
    }
    if(globalSingletons.inputState.keyboardState.keys[vkrt::VKRT_S_KEY].down) {
        printf("S key pressed!\n");
    }
    if(globalSingletons.inputState.keyboardState.keys[vkrt::VKRT_D_KEY].down) {
        printf("D key pressed!\n");
    }
}