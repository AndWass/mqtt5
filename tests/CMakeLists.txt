
add_executable(mqtt5-tests
    main.cpp
    integer.cpp
    string.cpp
    serialize.cpp
    binary.cpp
    packet.cpp
    connect.cpp
    connack.cpp
)

add_executable(mqtt5-v2-tests
    main.cpp

    v2/fixed_int.cpp
    v2/string.cpp
    v2/varlen_int.cpp
    v2/binary.cpp
    v2/header.cpp
    v2/properties.cpp
    v2/connect.cpp
    v2/optional_inplace_deserializer.cpp
)

target_link_libraries(mqtt5-tests PRIVATE 
    CONAN_PKG::doctest
    mqtt5
)

target_link_libraries(mqtt5-v2-tests PRIVATE 
    CONAN_PKG::doctest
    mqtt5
)
if(MSVC)
    target_compile_definitions(mqtt5-tests PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1 _WIN32_WINNT=0x0601)
    target_compile_definitions(mqtt5-v2-tests PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1 _WIN32_WINNT=0x0601)
endif()