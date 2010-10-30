#!/bin/bash

BIN="product/ripe"

#$BIN $FLAGS -b test/test_cgi.rip -o test/test_cgi.exe
#$BIN $FLAGS -b test/test_sdl.rip -o test/test_sdl.exe
#$BIN $FLAGS -b test/test_speech.rip -o test/test_speech.exe
#$BIN $FLAGS -b test/test_pthread.rip -o test/test_pthread.exe
$BIN $FLAGS -b -m Json test/test_json.rip -o test/test_json.exe
$BIN $FLAGS -b -m Xml test/test_xml.rip -o test/test_xml.exe
