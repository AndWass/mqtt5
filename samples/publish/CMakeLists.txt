add_executable(sample-publish sample-publish.cpp)
target_link_libraries(sample-publish PRIVATE mqtt5)

if(WIN32)
    target_compile_definitions(sample-publish PRIVATE _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
endif()