// Copyright (C) 2008  Maksim Sipos <msipos@mailc.net>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef CLIB_H
#define CLIB_H

// General
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Data types
#include <iso646.h>
#include <inttypes.h>
#include <stdbool.h>

typedef int64_t int64;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef int32_t int32;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int8_t int8;
typedef uint8_t uint8;
typedef uint32_t unichar; // Unicode character
typedef unsigned int uint;
typedef uintptr_t uintptr;
typedef uint16_t error;

// Errors
#define ERROR_OK              0
#define ERROR_UTF8_INVALID    1
#define ERROR_UTF8_LIMITED    2
#define ERROR_UTF8_SEEK       3

// Assertions
#include <assert.h>
#define assert_never()  assert(false)

#define ATTR_NORETURN __attribute__((noreturn))

// CLIB
#include "array.h"
#include "mem.h"
#include "hash.h"
#include "dict.h"
#include "utf8.h"



#endif
