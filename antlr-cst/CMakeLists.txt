cmake_minimum_required(VERSION 3.18)
project(rx_antlr_cst LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
set(ANTLR_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ANTLR_BUILD_STATIC ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    antlr4_cpp_source
    URL https://github.com/antlr/antlr4/archive/refs/tags/4.13.2.zip
    SOURCE_SUBDIR runtime/Cpp
)
FetchContent_MakeAvailable(antlr4_cpp_source)

file(GLOB ANTLR_GENERATED_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/generated/*.cpp"
)

add_executable(rx-cst
    src/main.cpp
    ${ANTLR_GENERATED_SOURCES}
)
target_include_directories(rx-cst PRIVATE src/generated)
target_link_libraries(rx-cst PRIVATE antlr4_static)
