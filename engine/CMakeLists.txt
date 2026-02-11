cmake_minimum_required(VERSION 3.28)

project(VkrtEngine VERSION 1.0.0 LANGUAGES CXX)

# Add Vulkan to be used by SDL3
find_package(Vulkan REQUIRED)
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

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

# Compile and copy shaders/ to final build, allows for shaders to be included at runtime-time
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.slang")

# For every shader file, create a rule on how to build it
foreach(shader ${SHADER_FILES})
    # Strip directory and final file extension
    get_filename_component(filename ${shader} NAME_WE)

    # Add command to compile the slang shader to spv
    add_custom_command(
        OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${filename}.spv 
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
        COMMAND ${SLANGC_EXECUTABLE} -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${filename}.spv -- ${shader}
        DEPENDS ${shader}
    )

    # Create a list of spv files for the stamp to depend on
    list(APPEND SPV_FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${filename}.spv)
endforeach()

# Use a stamp system with glob_recurse to have some degree of automatic rebuilding of shaders
# Note: This will not delete previously built shaders from bin/shaders if removed from engine/shaders
add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/shaders.stamp
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    COMMAND ${CMAKE_COMMAND} -E touch
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/shaders.stamp
    DEPENDS ${SPV_FILES}
    COMMENT "Updating runtime shaders"
)

add_custom_target(runtime_shader_copies DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/shaders.stamp)

# Engine requires shaders to be built
add_dependencies(engine runtime_shader_copies)