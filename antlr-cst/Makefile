ANTLR_VERSION := 4.13.2
ANTLR_JAR ?= .tools/antlr-$(ANTLR_VERSION)-complete.jar
ANTLR_URL := https://www.antlr.org/download/antlr-$(ANTLR_VERSION)-complete.jar
ANTLR_SHA256 := eae2dfa119a64327444672aff63e9ec35a20180dc5b8090b7a6ab85125df4d76
GRAMMAR_DIR := ../grammar
GEN_DIR := src/generated
BUILD_DIR := build
INPUT ?= samples/minimal.rx

.PHONY: generate cst clean

$(ANTLR_JAR):
	mkdir -p "$(dir $@)"
	curl -fL "$(ANTLR_URL)" -o "$@.download"
	echo "$(ANTLR_SHA256)  $@.download" | sha256sum -c -
	mv "$@.download" "$@"

generate: $(ANTLR_JAR)
	rm -rf "$(GEN_DIR)"
	mkdir -p "$(GEN_DIR)"
	java -jar "$(ANTLR_JAR)" -Dlanguage=Cpp -visitor -Xexact-output-dir -o "$(GEN_DIR)" "$(GRAMMAR_DIR)/RxLexer.g4"
	java -jar "$(ANTLR_JAR)" -Dlanguage=Cpp -visitor -Xexact-output-dir -lib "$(GEN_DIR)" -o "$(GEN_DIR)" "$(GRAMMAR_DIR)/Parser.g4"

cst: generate
	cmake -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=Release
	cmake --build "$(BUILD_DIR)" --parallel
	"$(BUILD_DIR)/rx-cst" "$(INPUT)"

clean:
	rm -rf "$(BUILD_DIR)" "$(GEN_DIR)"
