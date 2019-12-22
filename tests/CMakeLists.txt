
add_executable(mqtt5-tests
    main.cpp
    integer.cpp
    string.cpp
    serialize.cpp
    binary.cpp
    packet.cpp
    connect.cpp
)
target_link_libraries(mqtt5-tests PRIVATE 
    CONAN_PKG::doctest
    mqtt5
)