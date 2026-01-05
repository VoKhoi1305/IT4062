#!/bin/bash

# Build script for client_v2

CC=gcc
CFLAGS="-Wall -g $(pkg-config --cflags gtk+-3.0)"
LIBS="$(pkg-config --libs gtk+-3.0) -lpthread"

SRC_DIR=src
INC_DIR=include
OBJ_DIR=obj
TARGET=client_gtk_v2

# Create obj directory
mkdir -p $OBJ_DIR

# Compile all source files
echo "Compiling source files..."
for src in $SRC_DIR/*.c; do
    filename=$(basename "$src" .c)
    echo "  - Compiling $filename.c..."
    $CC $CFLAGS -I$INC_DIR -c "$src" -o "$OBJ_DIR/$filename.o"
    if [ $? -ne 0 ]; then
        echo "Error compiling $src"
        exit 1
    fi
done

# Link object files
echo "Linking..."
$CC $OBJ_DIR/*.o -o $TARGET $LIBS

if [ $? -eq 0 ]; then
    echo "✅ Build successful! Executable: ./$TARGET"
else
    echo "❌ Linking failed"
    exit 1
fi
