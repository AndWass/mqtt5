cmake_minimum_required(VERSION 3.8)
project(mqtt5)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE
    CONAN_PKG::span-lite
    CONAN_PKG::boost
    CONAN_PKG::utfcpp
    CONAN_PKG::stl_interfaces
)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

if(MQTT5_BUILD_TESTS)
    add_subdirectory(tests)
endif()