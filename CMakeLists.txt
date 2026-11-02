cmake_minimum_required(VERSION 3.18)

project(rx_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

# Use the exact ANTLR version that generated the checked-in C++ parser.
# Prefer the pinned submodule so the build does not require network access.
set(ANTLR_CPP_RUNTIME_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/antlr4/runtime/Cpp"
)

set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
set(ANTLR_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ANTLR_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ANTLR4_INSTALL OFF CACHE BOOL "" FORCE)

if(EXISTS "${ANTLR_CPP_RUNTIME_DIR}/CMakeLists.txt")
    add_subdirectory(
        "${ANTLR_CPP_RUNTIME_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/antlr4-runtime"
    )
else()
    # Allow a preinstalled, exactly matching runtime as a fallback when
    # the source submodule is not available in the build environment.
    find_package(antlr4-runtime 4.13.2 EXACT CONFIG QUIET)
    if(NOT TARGET antlr4_static)
        message(FATAL_ERROR
            "ANTLR 4.13.2 C++ Runtime was not found. Initialize the pinned "
            "submodule with: git submodule update --init vendor/antlr4"
        )
    endif()
endif()

set(RX_ANTLR_GENERATED_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler/build/generated"
)

if(NOT EXISTS "${RX_ANTLR_GENERATED_DIR}/RxParser.cpp")
    message(FATAL_ERROR
        "ANTLR generated sources were not found in ${RX_ANTLR_GENERATED_DIR}. "
        "The generated C++ parser sources must be present before configuring."
    )
endif()

file(GLOB RX_ANTLR_GENERATED_SOURCES CONFIGURE_DEPENDS
    "${RX_ANTLR_GENERATED_DIR}/*.cpp"
)

if(NOT RX_ANTLR_GENERATED_SOURCES)
    message(FATAL_ERROR
        "No ANTLR C++ source files were found in ${RX_ANTLR_GENERATED_DIR}."
    )
endif()

add_library(rx_compiler STATIC
    compiler/src/ast/ast.cpp
    compiler/src/frontend/astbuilder.cpp
    compiler/src/semantic/checker.cpp
    compiler/src/semantic/collector.cpp
    compiler/src/semantic/constevaluator.cpp
    compiler/src/semantic/declaration.cpp
    compiler/src/semantic/scope.cpp
    compiler/src/semantic/type.cpp
    ${RX_ANTLR_GENERATED_SOURCES}
)

add_library(rx::compiler ALIAS rx_compiler)

target_include_directories(rx_compiler
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/compiler/include>"
        "$<BUILD_INTERFACE:${RX_ANTLR_GENERATED_DIR}>"
)

target_link_libraries(rx_compiler
    PUBLIC
        antlr4_static
)

target_compile_features(rx_compiler PUBLIC cxx_std_17)

# Command-line entry point used by config.mk and the Online Judge.
add_executable(rx_compiler_cli
    compiler/src/main.cpp
)
target_link_libraries(rx_compiler_cli PRIVATE rx::compiler)

if(BUILD_TESTING)
    add_executable(astbuilder_current_test
        compiler/tests/astbuilder_current_test.cpp
    )
    target_link_libraries(astbuilder_current_test PRIVATE rx::compiler)

    add_test(
        NAME astbuilder_current_test
        COMMAND astbuilder_current_test
    )

    add_executable(semantic_first_pass_test
        compiler/tests/semantic_first_pass_test.cpp
    )
    target_link_libraries(semantic_first_pass_test PRIVATE rx::compiler)

    add_test(
        NAME semantic_first_pass_test
        COMMAND semantic_first_pass_test
    )
endif()
