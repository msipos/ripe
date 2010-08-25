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

#include "stringbuf.h"
#include <string.h>
#include <stdarg.h>

void sbuf_init(StringBuf* sbuf, const char* s)
{
  uint64 len = strlen(s);
  sbuf->str = mem_malloc(len + 1);
  sbuf->alloc_size = len + 1;
  sbuf->size = len + 1;
  strcpy(sbuf->str, s);
}

void sbuf_printf(StringBuf* sbuf, const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  // Calculate needed buffer
  int sz = vsnprintf(NULL, 0, format, ap) + 1;
  char tmpbuf[sz];
  vsnprintf(tmpbuf, sz, format, ap);
  tmpbuf[sz - 1] = 0;
  va_end(ap);
  
  uint tlen = strlen(tmpbuf);
  if (sbuf->size + tlen > sbuf->alloc_size){
    sbuf->alloc_size = (sbuf->size + tlen) * 2;
    sbuf->str = mem_realloc(sbuf->str, sbuf->alloc_size);
  }
  strcat(sbuf->str, tmpbuf);
  sbuf->size += tlen;
}

void sbuf_catc(StringBuf* sbuf, const char c)
{
  if (sbuf->size + 1 > sbuf->alloc_size){
    sbuf->alloc_size *= 2;
    sbuf->str = mem_realloc(sbuf->str, sbuf->alloc_size);
  }
  sbuf->str[sbuf->size - 1] = c;
  sbuf->str[sbuf->size] = 0;
  sbuf->size++;
}

void sbuf_clear(StringBuf* sbuf)
{
  sbuf->str[0] = 0;
  sbuf->size = 1;
}

void sbuf_deinit(StringBuf* sbuf)
{
  mem_free(sbuf->str);
}

