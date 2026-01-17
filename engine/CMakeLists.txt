cmake_minimum_required(VERSION 3.28)

project(VkrtEngine VERSION 1.0.0 LANGUAGES CXX)

# Find Vulkan
find_package(Vulkan)

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

# Add *.cpp to a list of sources
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/engine/src/*.cpp)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "^third_party/.*$")

# Add engine directory as a library
add_library(engine STATIC ${ENGINE_SOURCES})

# NOTE: In the future, I'd consider splitting the engine/ folder into include/ and src/
target_include_directories(engine 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/asset
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ecs
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vk-bootstrap/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/VulkanMemoryAllocator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/external/glm/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/external/tinygltf)

target_link_libraries(engine 
    PRIVATE Vulkan::Vulkan
    SDL3::SDL3 
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
    glm::glm
    tinygltf)