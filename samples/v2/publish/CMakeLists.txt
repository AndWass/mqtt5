add_executable(sample-v2-publish sample-v2-publish.cpp)
target_link_libraries(sample-v2-publish PRIVATE mqtt5)

if(WIN32)
    target_compile_definitions(sample-v2-publish PRIVATE _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
endif()