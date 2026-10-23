cmake_minimum_required(VERSION 3.18)
project(rx_ast_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Match the ANTLR version used to generate the checked-in parser sources.
set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
set(ANTLR_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ANTLR_BUILD_STATIC ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    antlr4_cpp_source
    URL https://github.com/antlr/antlr4/archive/refs/tags/4.13.2.zip
    SOURCE_SUBDIR runtime/Cpp
)
FetchContent_MakeAvailable(antlr4_cpp_source)

set(ANTLR_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/generated")
if(NOT EXISTS "${ANTLR_GENERATED_DIR}/RxParser.cpp")
    message(FATAL_ERROR
        "ANTLR generated sources were not found in ${ANTLR_GENERATED_DIR}. "
        "Regenerate the parser before building the AST test."
    )
endif()

file(GLOB ANTLR_GENERATED_SOURCES CONFIGURE_DEPENDS
    "${ANTLR_GENERATED_DIR}/*.cpp"
)

add_executable(astbuilder_current_test
    astbuilder_current_test.cpp
    ../src/ast/ast.cpp
    ../src/frontend/astbuilder.cpp
    ${ANTLR_GENERATED_SOURCES}
)

target_include_directories(astbuilder_current_test PRIVATE
    ../include
    "${ANTLR_GENERATED_DIR}"
)

target_link_libraries(astbuilder_current_test PRIVATE antlr4_static)

enable_testing()
add_test(
    NAME astbuilder_current_test
    COMMAND astbuilder_current_test
)
