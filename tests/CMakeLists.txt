add_executable(dx_tests
        src/main.cpp)
target_link_libraries(dx_tests PRIVATE libdnxpp)
add_test(NAME DxInterpreterTests COMMAND dx_tests)

# Copy data directory from source tree to output directory
add_custom_command(TARGET dx_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data/ $<TARGET_FILE_DIR:dx_tests>/data/)

# Copy DLLs to output directory
if (WIN32)
    add_custom_command(TARGET dx_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:dx_tests> $<TARGET_FILE_DIR:dx_tests>
            COMMAND_EXPAND_LISTS)
endif ()