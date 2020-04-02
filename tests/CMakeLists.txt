add_executable(mqtt5-tests
    main.cpp

    fixed_int.cpp
    string.cpp
    varlen_int.cpp
    binary.cpp
    header.cpp
    properties.cpp
    connect.cpp
    optional_inplace_deserializer.cpp
)

target_link_libraries(mqtt5-tests PRIVATE 
    CONAN_PKG::doctest
    mqtt5
)
if(MSVC)
    target_compile_definitions(mqtt5-tests PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1 _WIN32_WINNT=0x0601)
endif()
