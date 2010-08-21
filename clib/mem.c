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

#include "clib.h"
#include "mem.h"
#include <string.h>
#include <stdarg.h>

#ifndef CLIB_GC
void* mem_calloc(size_t sz)
{
  void* p = calloc(1, sz);
  if (p == NULL) abort();
  return p;
}

void* mem_malloc(size_t sz)
{
  void* p = malloc(sz);
  if (p == NULL) abort();
  return p;
}

void* mem_realloc(void* p, size_t sz)
{
  void* t = realloc(p, sz);
  if (t == NULL) abort();
  return t;
}
#endif

#ifndef CLIB_GC
char* mem_strdup(const char* s)
{
  char* d = strdup(s);
  if (d == NULL) abort();
  return d;
}
#endif

char* mem_asprintf(char* format, ...)
{
  #ifdef CLIB_GC
  char tmpbuf[1024];
  va_list ap;
  va_start(ap, format);
  vsnprintf(tmpbuf, 1023, format, ap);
  tmpbuf[1023] = 0;
  va_end(ap);
  char* str = (char*) mem_malloc(strlen(tmpbuf) + 1);
  strcpy(str, tmpbuf);
  return str;
  #else
  va_list args;
  va_start(args, format);
  char* s;
  if (vasprintf(&s, format, args) == -1) abort();
  va_end(args);
  return s;
  #endif
}

