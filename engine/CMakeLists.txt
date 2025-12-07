cmake_minimum_required(VERSION 3.11)

project(VkrtEngine VERSION 1.0.0 LANGUAGES CXX)

# @note Okay, okay, perhaps external/ should be in engine/, but I really wanted external/ to be top-level

# Add SDL as a subdirectory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL ${CMAKE_CURRENT_BINARY_DIR}/SDL EXCLUDE_FROM_ALL)

# Add vk-bootstrap as a subdirectory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/vk-bootstrap ${CMAKE_CURRENT_BINARY_DIR}/vk-bootstrap)

# Add engine directory as a library
add_library(engine STATIC ${ENGINE_SOURCES})

# Add *.cpp to a list of sources
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/engine/*.cpp)
target_sources(engine PUBLIC ${ENGINE_SOURCES})

# NOTE: In the future, I'd consider splitting the engine/ folder into include/ and src/
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL/include)

target_link_libraries(engine PRIVATE SDL3::SDL3 vk-bootstrap::vk-bootstrap)