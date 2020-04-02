cmake_minimum_required(VERSION 3.8)
project(mqtt5)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE
    CONAN_PKG::span-lite
    CONAN_PKG::boost
    CONAN_PKG::p0443
)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

if(MQTT5_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(MQTT5_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()