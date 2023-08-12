cmake_minimum_required(VERSION 3.26)

set(libdnxpp_VERSION 1.0.1)

project(libdnxpp
        VERSION ${libdnxpp_VERSION}
        DESCRIPTION "Reimplementation of the Diannex Interpreter for Modern C++ (C++20/23)"
        HOMEPAGE_URL "https://github.com/PeriBooty/libdnxpp"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif ()

option(BUILD_SAMPLE "Builds a sample program that uses the interepreter" OFF)
option(USE_FMTLIB "Use fmtlib/fmt to provide <format> functionality" OFF)

set(ZLIB_USE_STATIC_LIBS ON)
find_package(ZLIB REQUIRED)
if (USE_FMTLIB)
    find_package(fmt CONFIG REQUIRED)
endif ()

add_library(libdnxpp SHARED
        include/diannex/common.hpp
        include/diannex/exceptions.hpp
        include/diannex/models.hpp
        include/diannex/utils.hpp
        include/diannex/utils/BinaryReader.hpp
        include/diannex/utils/DxStack.hpp
        include/diannex/internal/DxValueConcepts.hpp
        include/diannex/DxInstructions.hpp
        include/diannex/DxData.hpp
        include/diannex/DxValue.hpp
        include/diannex/DxInterpreter.hpp
        src/DxData.cpp
        src/DxValue.cpp
        src/DxInterpreter.cpp
        src/DxInterpreterImpl.cpp
        src/utils/BinaryReader.cpp
        src/internal/DxDefinitionInstance.cpp
)
add_library(Diannex::libdnxpp ALIAS libdnxpp)
target_compile_features(libdnxpp PUBLIC cxx_std_23)
target_include_directories(libdnxpp
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/diannex>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_ICLUDEDIR}/diannex>)
target_link_libraries(libdnxpp PRIVATE ZLIB::ZLIB)

if (USE_FMTLIB)
    find_package(fmt CONFIG REQUIRED)
    target_link_libraries(libdnxpp PUBLIC fmt::fmt)
    target_compile_definitions(libdnxpp PUBLIC -DUSE_FMTLIB)
endif ()

configure_package_config_file(cmake/config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/libdnxpp-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/libdnxpp
        NO_SET_AND_CHECK_MACRO)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/libdnxpp-config-version.cmake
        VERSION ${libdnxpp_VERSION}
        COMPATIBILITY SameMajorVersion)
install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/libdnxpp-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/libdnxpp-config-version.cmake
        DESTINATION
        ${CMAKE_INSTALL_DATADIR}/libdnxpp)

install(TARGETS libdnxpp EXPORT libdnxpp-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT libdnxpp-targets
        NAMESPACE Diannex::
        DESTINATION ${CMAKE_INSTALL_DATADIR}/libdnxpp)

if (BUILD_SAMPLE)
    add_subdirectory(sample/)
endif ()

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests/)
endif ()