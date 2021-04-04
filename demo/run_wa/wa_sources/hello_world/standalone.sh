#!/bin/bash

#e.g. hello.cpp
NAME=$1
EX=$2

# emcc -s EMIT_EMSCRIPTEN_METADATA -O3 $NAME.$EX -o $NAME.wasm && wasm2wat $NAME.wasm -o $NAME.wast
emcc -O3 $NAME.$EX -o $NAME.wasm && wasm2wat $NAME.wasm -o $NAME.wast
