cmake_minimum_required(VERSION 3.26)
project(libdnxpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

option(BUILD_SAMPLE "Builds a sample program that uses the interepreter" ON)

set(ZLIB_USE_STATIC_LIBS ON)
find_package(ZLIB REQUIRED)

add_library(libdnxpp SHARED
        include/diannex/common.hpp
        include/diannex/exceptions.hpp
        include/diannex/models.hpp
        include/diannex/utils.hpp
        include/diannex/utils/BinaryReader.hpp
        include/diannex/utils/generator.hpp
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
target_include_directories(libdnxpp PUBLIC include/ PRIVATE include/diannex)
target_link_libraries(libdnxpp PRIVATE ZLIB::ZLIB)

if (BUILD_SAMPLE)
    add_subdirectory(sample/)
endif ()