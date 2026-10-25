cmake_minimum_required(VERSION 3.18)
project(rx_semantic_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(semantic_first_pass_test
    ../semantic_first_pass_test.cpp
    ../../src/ast/ast.cpp
    ../../src/semantic/collector.cpp
    ../../src/semantic/type.cpp
    ../../src/semantic/declaration.cpp
    ../../src/semantic/constevaluator.cpp
    ../../src/semantic/scope.cpp
)

target_include_directories(semantic_first_pass_test PRIVATE
    ../../include
)

enable_testing()
add_test(
    NAME semantic_first_pass_test
    COMMAND semantic_first_pass_test
)
