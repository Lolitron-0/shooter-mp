#!/usr/bin/env bash

set -e

mkdir -p build

sudo cmake  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -DCANVAS_BUILD_TESTS=1 -G Ninja -B build -S . 
sudo cmake --build build --parallel 5 

cp build/compile_commands.json ./compile_commands.json 
./build/server/shooter-server ${@:1}
