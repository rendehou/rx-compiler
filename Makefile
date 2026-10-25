.DEFAULT_GOAL := test

include config.mk

PYTHON ?= python3
VERBOSE ?= false
# Directory paths below tests, joined with ':'; separate selections with ','.
FILTER ?=
# Optional single stage: semantic, ir, codegen, or optimization.
STAGE ?=
CLANG ?= clang-22
COMPILE_TIMEOUT ?= 30
RUN_TIMEOUT ?= 10

# Export commands as data, so shell quoting survives Make's recipe expansion.
export RX_TEST_BUILD = $(BUILD)
export RX_TEST_SEMANTIC = $(SEMANTIC)
export RX_TEST_IR = $(IR)
export RX_TEST_CODEGEN = $(CODEGEN)
export RX_TEST_RUN = $(RUN)
export FILTER STAGE CLANG COMPILE_TIMEOUT RUN_TIMEOUT VERBOSE

.PHONY: test
test:
	@$(PYTHON) scripts/test.py

# Build and run the AST Builder unit test independently of the course test suite.
.PHONY: test-ast
test-ast:
	@cmake -S compiler/tests -B target/ast-test -DCMAKE_BUILD_TYPE=Debug
	@cmake --build target/ast-test --config Debug --parallel 2
	@ctest --test-dir target/ast-test -C Debug --output-on-failure

# Build and run the first-pass semantic unit test without ANTLR dependencies.
.PHONY: test-semantic
test-semantic:
	@cmake -S compiler/tests/semantic -B target/semantic-test -DCMAKE_BUILD_TYPE=Debug
	@cmake --build target/semantic-test --parallel 2
	@ctest --test-dir target/semantic-test --output-on-failure
