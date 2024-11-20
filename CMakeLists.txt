cmake_minimum_required(VERSION 3.5.0)
project(vulkanRaytracer)

set(CMAKE_CXX_STANDARD 17)

# Find Vulkan, GLFW, and GLM
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_executable(vulkanRaytracer src/main.cpp src/VulkanWrapper.cpp src/Window.cpp)
target_include_directories(vulkanRaytracer PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(vulkanRaytracer glfw vulkan)