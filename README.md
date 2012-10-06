Ripe
====

What is it?
-----------

A hobby programming language.

It is:

* compiled (through intermediate C step)
* hybrid dynamically and statically typed (optional types with a little type-inference)
* relatively fast when statically typed (in tight loops, outputted C code is as efficient as it gets)
* garbage collected (via Boehm GC)

It has:

* whitespace syntax (like Python but more terse)
* closures (well, anonymous blocks with access to outer scope constants)
* a tiny runtime library including a bit of everything (see modules directory)
* no interpreter and no interpreter lock (true threading implemented with pthread)
* fancy colorized build system

It generates tiny programs with the entire runtime compiled in (or it can be run in "scripting mode").  The only runtime requirement is the C library and Boehm GC.

The problems are:

* no generics (you must use dynamic typing to get over that)
* Boehm garbage collection is conservative, it has no knowledge of type information from the compiler

How to install
==============

Installation requirements
-------------------------

Linux (sorry, no support for Mac OSX or Windows right now).  It is known to build under Ubuntu 10.04 LTS and Ubuntu 12.04 LTS.

It only compiles under GCC as far as I know.

Installation requirements:
* libgc-dev
* bison
* flex
* python (for the build script)

Installation instructions
-------------------------

To install, simply run:

    ./build.py

(You may get some errors during compiling of optional modules, the build script will write out the modules that failed.)

To run tests simply run:

    ./run_test.sh
    
And you should see:

    Running language test...
    language test results: 90/90
    Running stdlib test...
    language test results: 106/106
    Running 2 file test...
    Test succeeded!

