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