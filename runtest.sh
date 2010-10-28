#!/bin/bash

FLAGS="-m Test -m Gsl"
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

echo "Running 2 file test..."
$BIN test/suite/file1.rip test/suite/file2.rip
