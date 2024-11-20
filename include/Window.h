#ifndef WINDOW_H
#define WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window{
public:
    void init();
    void cleanup();

    bool shouldClose();
    void pollEvents();

    GLFWwindow* getWindow();
    
private:
    GLFWwindow* window = nullptr;
};

#endif