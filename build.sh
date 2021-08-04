#!/bin/sh

CC=gcc
mkdir -p build/run

COMMON_FLAGS="-I src/include -g"
LIB_FLAGS="-shared -fPIE -fPIC"

$CC src/ctti/ctti.c -o build/run/ctti $COMMON_FLAGS
$CC src/renderer/renderer.c -o build/run/librenderer $COMMON_FLAGS $LIB_FLAGS
$CC src/game/game.c -o build/run/libgame $COMMON_FLAGS $LIB_FLAGS
$CC src/loader/loader.c src/loader/platform/unix.c src/loader/third_party/sds.c -o build/run/loader $COMMON_FLAGS -ldl -lpthread -lglfw
