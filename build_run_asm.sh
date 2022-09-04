#!/bin/bash
cd build
cmake --build .
cd src
./tama ./../../test/test.asm
