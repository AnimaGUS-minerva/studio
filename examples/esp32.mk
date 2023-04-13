SHELL := /bin/bash
STUDIO := $(shell realpath ../..)

TOOLCHAIN_XTENSA := $(STUDIO)/toolchain/xtensa
CLANG_ESP := $(HOME)/.espressif/tools/xtensa-esp32-elf-clang/esp-13.0.0-20211203-x86_64-unknown-linux-gnu

export CARGO_HOME := $(STUDIO)/target/cargo
export LIBCLANG_PATH := $(CLANG_ESP)/lib
export XARGO_RUST_SRC := $(TOOLCHAIN_XTENSA)/rust-build/rust-src-1.57.0.2/rust-src/lib/rustlib/src/rust/library
export PATH := $(CLANG_ESP)/bin:$(PATH)

esp32-build-module:
	@echo "Buidling 'target/xtensa-esp32-none-elf/release/*.a'"
	XTENSA_GCC=$(CLANG_ESP)/bin/xtensa-esp32-elf-gcc \
		cargo +esp xbuild --lib --release --target xtensa-esp32-none-elf $(CARGO_OPTS)

RIOT_BASE ?= $(STUDIO)/RIOT
esp32-build-riot:
	cd ./main && \
		CONTINUE_ON_EXPECTED_ERRORS=1 WERROR=0 BOARD=esp32-wroom-32 RIOTBASE=$(RIOT_BASE)  make
esp32-build-riot-micropython:
	cd ./micropython/ports/riot && \
		CUSTOM_BOARD=esp32 \
		CONTINUE_ON_EXPECTED_ERRORS=1 WERROR=0 BOARD=esp32-wroom-32 RIOTBASE=$(RIOT_BASE)  make

RIOT_ESP32_BIN ?= ./main.esp32.bin
esp32-build-flash:
	python3 $(TOOLCHAIN_XTENSA)/esptool/esptool.py --chip esp32 elf2image \
		-o $(RIOT_ESP32_BIN) main/bin/esp32-wroom-32/main.elf

esp32-run-riot: esp32-build-flash
	RIOT_ESP32_BIN=$(RIOT_ESP32_BIN) \
		cargo run --manifest-path ../runner/Cargo.toml esp32 $(EMU_TIMEOUT)
