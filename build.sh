#!/bin/bash

if [ ! -d build ]; then
    mkdir -p build;
fi

Compiler="clang"
SourceFile="code/linux.c"
OutputFile="build/snow"

CompileFlags=" \
    -O0 \
    -ffreestanding \
    -fno-stack-protector \
    -fpie \
    -std=c11 \
    -nostdlib \
    -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-unused-function \
    -o $OutputFile"

LinkFlags="
    -fuse-ld=lld \
    -Wl,-nostdlib \
    -Wl,-e,LinuxEntry"

$Compiler $CompileFlags $LinkFlags $SourceFile

