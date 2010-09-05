#!/bin/bash

FLAGS="-m Test -m Gsl -m Sdl -m Math -m Gd"
BIN="product/ripe"

FILE=language.rip
echo "Running $FILE ..."
$BIN $FLAGS test/suite/$FILE

FILE=stdlib.rip
echo "Running $FILE ..."
$BIN $FLAGS test/suite/$FILE
