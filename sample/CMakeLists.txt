find_package(raylib CONFIG REQUIRED)

add_executable(dx_sample
        src/main.cpp
        src/Game.cpp
        src/Game.hpp)
target_link_libraries(dx_sample PRIVATE raylib libdnxpp)

add_custom_command(TARGET dx_sample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data/ $<TARGET_FILE_DIR:dx_sample>/data/)

if (WIN32)
    add_custom_command(TARGET dx_sample POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:dx_sample> $<TARGET_FILE_DIR:dx_sample>
            COMMAND_EXPAND_LISTS)
endif ()