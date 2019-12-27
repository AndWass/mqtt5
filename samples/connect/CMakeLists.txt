add_executable(sample-connect sample-connect.cpp)
target_link_libraries(sample-connect PRIVATE mqtt5)

if(WIN32)
    target_compile_definitions(sample-connect PRIVATE _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
endif()