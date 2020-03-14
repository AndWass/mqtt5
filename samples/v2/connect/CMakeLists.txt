add_executable(sample-v2-connect sample-v2-connect.cpp)
target_link_libraries(sample-v2-connect PRIVATE mqtt5)

if(WIN32)
    target_compile_definitions(sample-v2-connect PRIVATE _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
endif()