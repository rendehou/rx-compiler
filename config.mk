# C++ compiler under compiler/.  The test runner executes BUILD once before
# running the selected cases.
BUILD = cmake -S compiler -B compiler/build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build compiler/build --parallel 2

# Stage 1 entrypoint.  This is already wired into the official semantic-test
# harness, but rxc currently performs only lexing/parsing.  Keep this command
# and replace --check-syntax with the semantic driver mode when the AST and
# SemanticChecker are connected.
SEMANTIC = ./compiler/build/rxc --check-syntax {source}

# Code generation is outside the current stage.  Leaving these empty makes the
# runner report a clear configuration error if codegen/optimization tests are
# selected, instead of silently testing the bundled Rust reference compiler.
CODEGEN =
RUN =
