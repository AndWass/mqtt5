add_executable(sample-client-connect sample-client-connect.cpp)
target_link_libraries(sample-client-connect PRIVATE mqtt5)

if(WIN32)
    target_compile_definitions(sample-client-connect PRIVATE _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
endif()