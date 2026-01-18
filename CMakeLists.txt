# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(VulkanRaytracer VERSION 1.0 LANGUAGES CXX)

# Separate binary from build files
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add engine directory as a library (glob sources, add library, etc)
add_subdirectory(engine)

# Add executable
add_executable(VulkanRaytracer app/main.cpp app/App.cpp)

# Link libraries
target_link_libraries(VulkanRaytracer
    PUBLIC engine
)

# Copy assets/ to final build, allows for new and updated assets to be included at build-time
file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/assets/*")

add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/assets.stamp
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/
    COMMAND ${CMAKE_COMMAND} -E touch
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/assets.stamp
    DEPENDS ${ASSET_FILES}
    COMMENT "Updating runtime assets"
)

add_custom_target(runtime_asset_copies DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/assets.stamp)

add_dependencies(VulkanRaytracer runtime_asset_copies)