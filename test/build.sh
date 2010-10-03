#!/bin/bash

FLAGS="-m Curl -m Test -m Gsl -m Sdl -m MainLoop -m Math -m Gd -m Speech -m Pthread"
BIN="product/ripe"

$BIN $FLAGS -b test/test_cgi.rip -o test/test_cgi.exe
$BIN $FLAGS -b test/test_sdl.rip -o test/test_sdl.exe
$BIN $FLAGS -b test/test_speech.rip -o test/test_speech.exe
$BIN $FLAGS -b test/test_pthread.rip -o test/test_pthread.exe
