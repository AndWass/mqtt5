if(MSVC)
    add_compile_options(/bigobj)
    add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()
add_subdirectory(client-connect)
add_subdirectory(connect)
add_subdirectory(subscribe)
add_subdirectory(publish)