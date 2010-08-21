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

#ifndef MEM_H
#define MEM_H

#include "clib.h"

#ifdef CLIB_GC
  #ifndef NDEBUG
    #define GC_DEBUG
  #endif
  #include <gc/gc.h>

  #define mem_malloc(sz) GC_MALLOC(sz)
  #define mem_calloc(sz) GC_MALLOC(sz)
  #define mem_realloc(p, sz) GC_REALLOC(p, sz)
  #define mem_strdup(p) GC_STRDUP(p)
  #define mem_free(p) GC_FREE(p)
#else
  void* mem_malloc(size_t sz);
  void* mem_calloc(size_t sz);
  void* mem_realloc(void* p, size_t sz);
  #define mem_free(p)   free(p)
  char* mem_strdup(const char* s);
#endif

// General functions
#define mem_new(T)    ((T *) mem_malloc(sizeof(T)))
char* mem_asprintf(char* format, ...);

#endif
