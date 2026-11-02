# The Online Judge invokes these commands through "make test".
# BUILD compiles the C++ compiler once. If Cargo is available, it also builds
# the Rust reference runtime used by the default IR/CODEGEN commands.

BUILD = cmake -S . -B target/cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF && \
    cmake --build target/cmake --target rx_compiler_cli --parallel 2 && \
    if command -v cargo >/dev/null 2>&1; then \
        CARGO_PROFILE_RELEASE_LTO=true cargo build --quiet --locked --release \
            --manifest-path crates/rx/Cargo.toml \
            --target-dir target/reference \
            --target riscv32im-unknown-none-elf; \
    fi && : > {runtime}

# The C++ compiler is used for semantic tests.
# Exit 0 to accept {source}, 1 to reject it.
SEMANTIC = target/cmake/rx_compiler_cli {source}

# The following commands retain the template's Rust reference implementation
# for IR/codegen testing. They require the Rust target and Clang 22.
IR = RUST_MIN_STACK=16777216 RX_SOURCE={source} $(REFERENCE_RUSTC) --crate-type=staticlib \
    --emit=llvm-ir={output},link={output}.a -C opt-level=2 -C lto=fat \
    crates/rx/src/entry.rs

CODEGEN = RUST_MIN_STACK=16777216 RX_SOURCE={source} $(REFERENCE_RUSTC) --crate-type=staticlib \
    --emit=asm={output},link={output}.a -C opt-level=2 -C lto=fat \
    -C llvm-args=-riscv-no-aliases crates/rx/src/entry.rs && \
    $(PYTHON) scripts/strip_asm_debug.py {output}

RUN = xmake run -P vendor/REIMU reimu --memory=256M --stack=1M \
    -f {output},{runtime} -o {stdout} -p {profile} 1>&2

# Rust reference helper; remove once all stages use the C++ compiler.
# v0 symbols avoid quoted section names that REIMU does not recognize.
REFERENCE_RUSTC = rustc \
    --edition=2021 \
    --target=riscv32im-unknown-none-elf \
    --crate-name=rx_test \
    -Awarnings -Aarithmetic_overflow \
    -C overflow-checks=off -C panic=abort -C symbol-mangling-version=v0 \
    --extern rx=target/reference/riscv32im-unknown-none-elf/release/librx.rlib \
    -L dependency=target/reference/riscv32im-unknown-none-elf/release/deps
