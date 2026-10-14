# asm-aarch64

An AArch64 assembler written in C.

## Overview

The goal of this project is to implement an assembler that translates
AArch64 assembly source code into machine code.

## Target Architecture

- Target: AArch64
- Development platform: Ubuntu x86-64
- Implementation language: C
- Compiler: GCC
- Reference tools: GNU Binutils
- Emulation: QEMU

## Planned Features

- Assembly source parsing
- Instruction encoding
- Label and branch resolution
- Object file generation
- Command-line interface
- Automated tests

## Build

```bash
mkdir -p build

gcc -Wall -Wextra -Wpedantic -Werror -std=c11 \
    -g -O0 -Iinclude \
    src/main.c \
    -o build/asm-aarch64