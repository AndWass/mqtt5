
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
)

target_link_libraries(mqtt5-tests PRIVATE 
    CONAN_PKG::doctest
    mqtt5
)

target_link_libraries(mqtt5-v2-tests PRIVATE 
    CONAN_PKG::doctest
    mqtt5
)