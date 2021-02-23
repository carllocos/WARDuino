#!/bin/bash
make clean
rm val.wasm
wat2wasm ex.wast -o val.wasm && wasmtime val.wasm
