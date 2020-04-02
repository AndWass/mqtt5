add_executable(sample-subscribe sample-subscribe.cpp)
target_link_libraries(sample-subscribe PRIVATE mqtt5)

if(TARGET CONAN_PKG::openssl)
    add_executable(sample-subscribe-ssl sample-subscribe-ssl.cpp)
    target_link_libraries(sample-subscribe-ssl PRIVATE mqtt5 CONAN_PKG::openssl)

    add_executable(sample-subscribe-ws-ssl sample-subscribe-ws-ssl.cpp)
    target_link_libraries(sample-subscribe-ws-ssl PRIVATE mqtt5 CONAN_PKG::openssl)
endif()

add_executable(sample-subscribe-ws sample-subscribe-ws.cpp)
target_link_libraries(sample-subscribe-ws PRIVATE mqtt5)

if(WIN32)
    target_compile_definitions(sample-subscribe PRIVATE _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
    target_compile_definitions(sample-subscribe-ssl PRIVATE _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
    target_compile_definitions(sample-subscribe-ws PRIVATE _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
endif()