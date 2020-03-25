add_executable(sample-v2-subscribe sample-v2-subscribe.cpp)
target_link_libraries(sample-v2-subscribe PRIVATE mqtt5)

if(WIN32)
    target_compile_definitions(sample-v2-subscribe PRIVATE _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
endif()