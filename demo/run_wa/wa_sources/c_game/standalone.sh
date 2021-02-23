#!/bin/bash

#e.g. hello.cpp
NAME=$1
EX=$2
echo CLEANING
echo ======================
rm $NAME.wasm $NAME.wast import.c $NAME.wasm.dbg
echo 
echo $NAME.$EX to WASM and WAST
echo ======================
# emcc -s EMIT_EMSCRIPTEN_METADATA -O3 $NAME.$EX -o $NAME.wasm && wasm2wat $NAME.wasm -o $NAME.wast
# emcc -O3 $NAME.$EX -o $NAME.wasm && wasm2wat $NAME.wasm -o $NAME.wast
# emcc -s SIDE_MODULE=1 -s NO_FILESYSTEM=1 -g4 --ignore-dynamic-linking -O3 $NAME.$EX -o $NAME.wasm && wasm2wat $NAME.wasm -o $NAME.wast
emcc $NAME.$EX -s SIDE_MODULE=1 -g4 --ignore-dynamic-linking -s WASM=1 -O3 -s NO_FILESYSTEM=1 -s ERROR_ON_UNDEFINED_SYMBOLS=0 -o $NAME.wasm && wasm2wat $NAME.wasm -o $NAME.wast

echo
echo Making $NAME.wasm.dbg
echo ======================
wat2wasm -v $NAME.wast >> $NAME.wasm.dbg


echo
echo creating import.c file
echo ========================
xxd -i $NAME.wasm > import.c
