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
#define strequal(x,y)  (0 == strcmp(x,y))

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

#define ATTR_NORETURN      __attribute__((noreturn))
#ifndef NOTHREADS
#define THREAD_LOCAL  __thread
#else
#define THREAD_LOCAL
#endif

///////////////////////////////////////////////////////////////////////
// array.c
///////////////////////////////////////////////////////////////////////

typedef struct {
  void* data;
  uint size;
  uint alloc_size;
} Array;

#define array_init(arr, T)  array_init2(arr, sizeof(T))
void array_init2(Array* arr, uint sz);
void array_deinit(Array* arr);

#define array_new(T)   array_new2(sizeof(T))
Array* array_new2(uint sz);
void array_delete(Array* arr);

#define array_data(arr, T)          ((T*) (arr)->data)
#define array_get(arr, T, i)        (((T *) (arr)->data)[i])
#define array_append(arr, value)  { array_expand(arr, sizeof(value)); \
                                    ((typeof(value) *) (arr)->data)[(arr)->size] = value; \
                                    (arr)->size++; }
#define array_pop(arr, T)         ({(arr)->size--; (((T*) (arr)->data)[(arr)->size]); })
void array_prepend(Array* arr, uint sz, void* el);
void array_expand(Array* arr, uint sz);
void array_clear(Array* arr);

///////////////////////////////////////////////////////////////////////
// dict.c
///////////////////////////////////////////////////////////////////////
// Get prime that is 2^power-(small number), and is bigger than n.
int64 map_prime(int64 n);

typedef uint64 (*DictHashFunc) (void* p);
typedef bool (*DictEqualFunc) (void* a, void* b);

typedef struct {
  uint16 key_size;
  uint16 key_size_align;
  uint16 value_size;
  uint16 value_size_align;
  uint16 total_size;
  uint64 size;
  uint64 alloc_size;
  DictHashFunc hash_func;
  DictEqualFunc equal_func;
  void* data;
} Dict;

void dict_init(Dict* d, uint16 key_size, uint16 value_size,
               DictHashFunc hash_func, DictEqualFunc equal_func);
Dict* dict_new(uint16 key_size, uint16 value_size, DictHashFunc hash_func,
               DictEqualFunc equal_func);

void dict_set(Dict* d, void* key, void* value);
bool dict_query(Dict* d, void* key, void* value);

// Hash & equality functions
uint64 dict_hash_string(void* key);
bool dict_equal_string(void* key1, void* key2);

uint64 dict_hash_uint32(void* key);
bool dict_equal_uint32(void* key1, void* key2);

uint64 dict_hash_uint64(void* key);
bool dict_equal_uint64(void* key1, void* key2);

// Direct bucket access (place = 0...alloc_size-1)
bool dict_has_bucket(Dict* d, int64 place);
void* dict_get_bucket_key(Dict* d, int64 place);
void* dict_get_bucket_value(Dict* d, int64 place);

///////////////////////////////////////////////////////////////////////
// hash.c
///////////////////////////////////////////////////////////////////////
uint32 hash_bytes(const char * data, int len);
#define hash_value(value)  hash_bytes((const char*) &value, sizeof(value))
#define hash_string(str)   hash_bytes(str, strlen(str))

///////////////////////////////////////////////////////////////////////
// mem.c
///////////////////////////////////////////////////////////////////////

#ifdef CLIB_GC
  #ifndef NDEBUG
    #define GC_DEBUG
  #endif
  #ifndef NOTHREADS
    #define GC_THREADS
  #endif
  #include <gc/gc.h>

  #define mem_malloc(sz) GC_MALLOC(sz)
  #define mem_malloc_atomic(sz) GC_MALLOC_ATOMIC(sz)
  #define mem_calloc(sz) GC_MALLOC(sz)
  #define mem_realloc(p, sz) GC_REALLOC(p, sz)
  #define mem_strdup(p) GC_STRDUP(p)
  #define mem_free(p) GC_FREE(p)
#else
  void* mem_malloc(size_t sz);
  void* mem_malloc_atomic(size_t sz);
  void* mem_calloc(size_t sz);
  void* mem_realloc(void* p, size_t sz);
  #define mem_free(p)   free(p)
  char* mem_strdup(const char* s);
#endif

// General functions
#define mem_new(T)    ((T *) mem_malloc(sizeof(T)))
char* mem_asprintf(char* format, ...);

///////////////////////////////////////////////////////////////////////
// path.c
///////////////////////////////////////////////////////////////////////

bool path_exists(const char* filename);
const char* path_get_app_dir();
const char* path_get_temp_dir();
const char* path_temp_name(const char* prefix, const char* suffix);

///////////////////////////////////////////////////////////////////////
// stringbuf.c
///////////////////////////////////////////////////////////////////////

typedef struct {
  char* str;
  uint64 alloc_size;
  uint64 size;
} StringBuf;

void sbuf_init(StringBuf* sbuf, const char* s);
void sbuf_printf(StringBuf* sbuf, const char* format, ...);
void sbuf_catc(StringBuf* sbuf, const char c);
void sbuf_cat(StringBuf* sbuf, const char* s);
void sbuf_ncat(StringBuf* sbuf, const char* s, int64 n);
void sbuf_ncpy(StringBuf* sbuf, const char* s, int64 n);
void sbuf_clear(StringBuf* sbuf);
void sbuf_deinit(StringBuf* sbuf);

///////////////////////////////////////////////////////////////////////
// utf8.c
///////////////////////////////////////////////////////////////////////

// Reads one unichar from the buffer given by *str, taking care
// not to read byte at limit or beyond it. Returns the unichar in
// out.
//
// If successful, it updates *str to point to the next codepoint (if any).
// If it fails, *str is left unchanged.
error utf8_read(const char** str, const char* limit, unichar* out);

// Write unichar c to the buffer given in *str, taking care not to write the
// byte at limit or beyond it.
//
// If successful, it updates *str_to point to the next codepoint (if any).
// If it fails, *str is left unchanged.
error utf8_write(char** str, const char* limit, unichar c);

#endif
