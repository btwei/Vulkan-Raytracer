#include "Window.h"

void Window::init(){
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(640, 480, "Vulkan Raytracer", nullptr, nullptr);
}

GLFWwindow* Window::getWindow(){
    return window;
}

bool Window::shouldClose(){
    return glfwWindowShouldClose(window);
}

void Window::pollEvents(){
    glfwPollEvents();
}

void Window::cleanup(){
    glfwDestroyWindow(window);

    glfwTerminate();
}