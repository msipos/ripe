#!/bin/bash

FLAGS="-m Test -m Gsl -m Sdl -m Math -m Gd"
BIN="product/ripe"

TEST=language
echo "Running $TEST test..."
RIPFILE=test/suite/$TEST.rip
EXEFILE=test/suite/$TEST
$BIN $FLAGS -b $RIPFILE -o $EXEFILE
./$EXEFILE

TEST=stdlib
echo "Running $TEST test..."
RIPFILE=test/suite/$TEST.rip
EXEFILE=test/suite/$TEST
$BIN $FLAGS -b $RIPFILE -o $EXEFILE
./$EXEFILE
