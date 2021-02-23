#!/bin/bash
rm val.wasm tictactoe.c
wat2wasm tictactoe.wast -o val.wasm && wasmtime val.wasm
