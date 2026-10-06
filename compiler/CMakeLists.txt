cmake_minimum_required(VERSION 3.20)
project(rxc_frontend LANGUAGES CXX)

include(CTest)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(RX_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(ANTLR_ROOT "${RX_PROJECT_ROOT}/../../.tools/antlr4-4.13.2"
    CACHE PATH "Directory containing the ANTLR 4.13.2 jar, headers, and library")
set(ANTLR_JAR "${ANTLR_ROOT}/antlr-4.13.2-complete.jar")
set(ANTLR_INCLUDE_DIR "${ANTLR_ROOT}/include/antlr4-runtime")
set(ANTLR_LIBRARY "${ANTLR_ROOT}/lib/libantlr4-runtime.a")

foreach(required_path IN ITEMS "${ANTLR_JAR}" "${ANTLR_INCLUDE_DIR}" "${ANTLR_LIBRARY}")
    if(NOT EXISTS "${required_path}")
        message(FATAL_ERROR "Missing ANTLR component: ${required_path}")
    endif()
endforeach()

find_package(Java 11 REQUIRED COMPONENTS Runtime)
find_package(Threads REQUIRED)

set(STAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/antlr-grammar")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(RX_LEXER_GRAMMAR "${STAGE_DIR}/RxLexer.g4")
set(RX_PARSER_GRAMMAR "${STAGE_DIR}/RxParser.g4")

add_custom_command(
    OUTPUT "${RX_LEXER_GRAMMAR}" "${RX_PARSER_GRAMMAR}"
    COMMAND "${CMAKE_COMMAND}"
        -DSOURCE_DIR=${RX_PROJECT_ROOT}/grammar
        -DOUTPUT_DIR=${STAGE_DIR}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PrepareAntlrGrammars.cmake"
    DEPENDS
        "${RX_PROJECT_ROOT}/grammar/Lexer.g4"
        "${RX_PROJECT_ROOT}/grammar/Parser.g4"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PrepareAntlrGrammars.cmake"
    VERBATIM
)

set(GENERATED_SOURCES
    "${GENERATED_DIR}/RxLexer.cpp"
    "${GENERATED_DIR}/RxParser.cpp"
    "${GENERATED_DIR}/RxParserBaseVisitor.cpp"
    "${GENERATED_DIR}/RxParserVisitor.cpp"
)
set(GENERATED_HEADERS
    "${GENERATED_DIR}/RxLexer.h"
    "${GENERATED_DIR}/RxParser.h"
    "${GENERATED_DIR}/RxParserBaseVisitor.h"
    "${GENERATED_DIR}/RxParserVisitor.h"
)

add_custom_command(
    OUTPUT ${GENERATED_SOURCES} ${GENERATED_HEADERS}
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GENERATED_DIR}"
    COMMAND "${Java_JAVA_EXECUTABLE}" -jar "${ANTLR_JAR}"
        -Dlanguage=Cpp -visitor -no-listener -package rx -Xexact-output-dir
        -o "${GENERATED_DIR}" "${RX_LEXER_GRAMMAR}"
    COMMAND "${Java_JAVA_EXECUTABLE}" -jar "${ANTLR_JAR}"
        -Dlanguage=Cpp -visitor -no-listener -package rx -Xexact-output-dir
        -lib "${GENERATED_DIR}" -o "${GENERATED_DIR}" "${RX_PARSER_GRAMMAR}"
    DEPENDS "${RX_LEXER_GRAMMAR}" "${RX_PARSER_GRAMMAR}" "${ANTLR_JAR}"
    VERBATIM
)

add_library(rx_generated_parser STATIC ${GENERATED_SOURCES} ${GENERATED_HEADERS})
target_compile_definitions(rx_generated_parser PUBLIC ANTLR4CPP_STATIC)
target_include_directories(rx_generated_parser PUBLIC
    "${GENERATED_DIR}"
    "${ANTLR_INCLUDE_DIR}"
)
target_link_libraries(rx_generated_parser PUBLIC "${ANTLR_LIBRARY}" Threads::Threads)

add_library(rx_frontend STATIC
    src/ast/ast.cpp
    src/frontend/diagnostic.cpp
    src/frontend/parser.cpp
    src/frontend/ast_builder.cpp
)
target_include_directories(rx_frontend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(rx_frontend PUBLIC rx_generated_parser)

add_executable(rxc src/main.cpp)
target_link_libraries(rxc PRIVATE rx_frontend)

if(BUILD_TESTING)
    add_executable(rx_ast_tests tests/ast_tests.cpp)
    target_link_libraries(rx_ast_tests PRIVATE rx_frontend)
    add_test(NAME rx_ast_tests COMMAND rx_ast_tests)
endif()
