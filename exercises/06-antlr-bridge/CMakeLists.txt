cmake_minimum_required(VERSION 3.20)
project(rx_antlr_ast_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(RX_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(ANTLR_ROOT "${RX_PROJECT_ROOT}/../../.tools/antlr4-4.13.2")
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

set(STAGE_DIR "${CMAKE_BINARY_DIR}/antlr-grammar")
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(RX_LEXER_GRAMMAR "${STAGE_DIR}/RxLexer.g4")
set(RX_PARSER_GRAMMAR "${STAGE_DIR}/RxParser.g4")

add_custom_command(
    OUTPUT "${RX_LEXER_GRAMMAR}" "${RX_PARSER_GRAMMAR}"
    COMMAND "${CMAKE_COMMAND}"
        -DSOURCE_DIR=${RX_PROJECT_ROOT}/grammar
        -DOUTPUT_DIR=${STAGE_DIR}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/PrepareAntlrGrammars.cmake"
    DEPENDS
        "${RX_PROJECT_ROOT}/grammar/Lexer.g4"
        "${RX_PROJECT_ROOT}/grammar/Parser.g4"
        "${CMAKE_CURRENT_SOURCE_DIR}/PrepareAntlrGrammars.cmake"
    VERBATIM
)

set(GENERATED_SOURCES
    "${GENERATED_DIR}/RxLexer.cpp"
    "${GENERATED_DIR}/RxParser.cpp"
    "${GENERATED_DIR}/RxParserBaseVisitor.cpp"
    "${GENERATED_DIR}/RxParserVisitor.cpp"
)

add_custom_command(
    OUTPUT ${GENERATED_SOURCES}
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

add_executable(rx_antlr_ast_example
    main.cpp
    antlr_ast.cpp
    ${GENERATED_SOURCES}
)
target_compile_definitions(rx_antlr_ast_example PRIVATE ANTLR4CPP_STATIC)
target_include_directories(rx_antlr_ast_example PRIVATE
    "${GENERATED_DIR}"
    "${ANTLR_INCLUDE_DIR}"
)
target_link_libraries(rx_antlr_ast_example PRIVATE
    "${ANTLR_LIBRARY}"
    Threads::Threads
)
