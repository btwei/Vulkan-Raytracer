#include "Window.h"
#include "VulkanWrapper.h"

int main() {

    Window window;
    window.init();

    VulkanWrapper vulkanWrapper;
    vulkanWrapper.init(window.getWindow());

    while(!window.shouldClose()){
        window.pollEvents();
    }

    return 0;
}