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

#ifndef STRINGBUF_H
#define STRINGBUF_H

#include "clib.h"

typedef struct {
  char* str;
  uint64 alloc_size;
  uint64 size;
} StringBuf;

void sbuf_init(StringBuf* sbuf, const char* s);
void sbuf_printf(StringBuf* sbuf, const char* format, ...);
void sbuf_catc(StringBuf* sbuf, const char c);
void sbuf_clear(StringBuf* sbuf);
void sbuf_deinit(StringBuf* sbuf);

#endif
