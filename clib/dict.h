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

#ifndef DICT_H
#define DICT_H

#include "clib.h"

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


#endif
