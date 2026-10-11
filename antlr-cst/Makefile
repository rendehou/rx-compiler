ANTLR_VERSION := 0.6.0
ANTLR_JAR ?= .tools/antlr4-4.13.3-SNAPSHOT-complete.jar
ANTLR_URL := https://github.com/antlr4rust/antlr4/releases/download/v$(ANTLR_VERSION)/antlr4-4.13.3-SNAPSHOT-complete.jar
ANTLR_SHA256 := 391e77311794f7c8fed2bc6d291f54b04725e37456d79e1981edfd3fc919b677
GRAMMAR_DIR := ../grammar
PREP_DIR := .tools/grammar
GEN_DIR := src/generated
INPUT ?= samples/minimal.rx

.PHONY: prepare generate cst clean

$(ANTLR_JAR):
	mkdir -p "$(dir $@)"
	curl -fL "$(ANTLR_URL)" -o "$@.download"
	echo "$(ANTLR_SHA256)  $@.download" | sha256sum -c -
	mv "$@.download" "$@"

prepare:
	python3 scripts/prepare_grammar.py "$(GRAMMAR_DIR)" "$(PREP_DIR)"

generate: $(ANTLR_JAR) prepare
	mkdir -p "$(GEN_DIR)"
	java -jar "$(ANTLR_JAR)" -Dlanguage=Rust -Xexact-output-dir -o "$(GEN_DIR)" "$(PREP_DIR)/RxLexer.g4"
	java -jar "$(ANTLR_JAR)" -Dlanguage=Rust -visitor -Xexact-output-dir -lib "$(GEN_DIR)" -o "$(GEN_DIR)" "$(PREP_DIR)/RxParser.g4"

cst: generate
	cargo run -- "$(INPUT)"

clean:
	find "$(GEN_DIR)" -maxdepth 1 -type f -name '*.rs' ! -name mod.rs -delete
	rm -f "$(GEN_DIR)"/*.tokens "$(GEN_DIR)"/*.interp
	rm -rf "$(PREP_DIR)"
