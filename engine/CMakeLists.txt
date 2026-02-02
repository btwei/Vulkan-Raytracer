cmake_minimum_required(VERSION 3.28)

project(VkrtEngine VERSION 1.0.0 LANGUAGES CXX)

# Add Vulkan to be used by SDL3
find_package(Vulkan REQUIRED)

# Add Volk as a subdirectory (requires Vulkan SDK to be set in environment variables)
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
elseif(UNIX)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR)
endif()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/volk)

# Add SDL as a subdirectory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL EXCLUDE_FROM_ALL)

# Add vk-bootstrap as a subdirectory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/vk-bootstrap)

# Add VMA as a subdirectory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/VulkanMemoryAllocator)

# Add GLM as a subdirectory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm)

# Add tinygltf as a subdirectory
option(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE)
option(TINYGLTF_INSTALL OFF CACHE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf)

# Add DearImGUI as a library 
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PRIVATE Vulkan::Vulkan SDL3::SDL3)

# Add *.cpp to a list of sources
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/engine/src/*.cpp)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "^third_party/.*$")

# Add engine directory as a library
add_library(engine STATIC ${ENGINE_SOURCES})

# Disable minmax macros from <windows.h> when compiling on Windows
target_compile_definitions(engine PUBLIC NOMINMAX)

target_include_directories(engine 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/asset
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ecs
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/volk
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vk-bootstrap/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/VulkanMemoryAllocator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm
)

target_link_libraries(engine 
    PUBLIC Vulkan::Vulkan
    PRIVATE volk::volk
    SDL3::SDL3 
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
    glm::glm
    tinygltf
    imgui)