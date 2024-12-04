#include "Window.h"
#include "VulkanWrapper.h"

int main() {
    try {
        Window window;
        window.init();

        VulkanWrapper vulkanWrapper;
        vulkanWrapper.init(window.getWindow());

        while(!window.shouldClose()){
            window.pollEvents();
        }

        vulkanWrapper.cleanup();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}