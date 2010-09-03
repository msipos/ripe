#!/bin/bash

FLAGS="-m Test -m Gsl -m Sdl -m Math -m TextFile -m Gd"
BIN="product/ripe"

FILE=operators.rip
echo "Running $FILE ..."
$BIN $FLAGS test/suite/$FILE

FILE=stdlib.rip
echo "Running $FILE ..."
$BIN $FLAGS test/suite/$FILE

FILE=varargs.rip
echo "Running $FILE ..."
$BIN $FLAGS test/suite/$FILE

FILE=basic-class.rip
echo "Running $FILE ..."
$BIN $FLAGS test/suite/$FILE

FILE=c-expr.rip
echo "Running $FILE ..."
$BIN $FLAGS test/suite/$FILE

FILE=cdata-class.rip
echo "Running $FILE ..."
$BIN $FLAGS test/suite/$FILE

FILE=sdl.rip
echo "Running $FILE ..."
$BIN $FLAGS test/suite/$FILE

FILE=draw.rip
echo "Running $FILE ..."
$BIN $FLAGS test/suite/$FILE
