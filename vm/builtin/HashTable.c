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

#include "vm/vm.h"

static uint64 query(BucketType* buckets, Value* keys, uint64 size, Value key)
{
  const uint64 h = op_hash(key);
  bool first_empty = false;
  uint64 first_empty_place;

  for (uint64 idx = 0; idx < size; idx++){
    // Quadratic probing
    uint64 place = (h + idx*idx) % size;

    switch(buckets[place]){
      case BUCKET_EMPTY:
        if (first_empty) return first_empty_place;
        return place;
      case BUCKET_WAS_FULL:
        if (not first_empty){
          first_empty = true;
          first_empty_place = place;
        }
        break;
      case BUCKET_FULL:
        if (op_equal2(key, keys[place])) return place;
        break;
    }
  }
  if (first_empty) return first_empty_place;
  assert_never();
}

static void rehash(HashTable* ht, bool do_values)
{
  uint64 new_alloc_size = map_prime(ht->alloc_size);
  BucketType* new_buckets = mem_malloc(sizeof(BucketType) * new_alloc_size);
  Value* new_keys = mem_malloc(sizeof(Value) * new_alloc_size);
  Value* new_values;
  if (do_values) new_values = mem_malloc(sizeof(Value) * new_alloc_size);

  const uint64 alloc_size = ht->alloc_size;
  for (uint64 i = 0; i < alloc_size; i++){
    if (ht->buckets[i] == BUCKET_FULL){
      const Value key = ht->keys[i];
      const uint64 place = query(new_buckets, new_keys, new_alloc_size, key);
      new_buckets[place] = BUCKET_FULL;
      new_keys[place] = key;
      if (do_values) new_values[place] = ht->values[i];
    }
  }

  ht->alloc_size = new_alloc_size;
  mem_free(ht->buckets); ht->buckets = new_buckets;
  mem_free(ht->keys); ht->keys = new_keys;
  if (do_values){
    mem_free(ht->values); ht->values = new_values;
  }
}

bool ht_query(HashTable* ht, Value key)
{
  uint64 place = query(ht->buckets, ht->keys, ht->alloc_size, key);
  return ht->buckets[place] == BUCKET_FULL;
}

bool ht_query2(HashTable* ht, Value key, Value* value)
{
  uint64 place = query(ht->buckets, ht->keys, ht->alloc_size, key);
  if (ht->buckets[place] != BUCKET_FULL) return false;
  if (value != NULL) *value = ht->values[place];
  return true;
}

bool ht_remove(HashTable* ht, Value key)
{
  uint64 place = query(ht->buckets, ht->keys, ht->alloc_size, key);
  if (ht->buckets[place] == BUCKET_FULL){
    ht->size--;
    ht->buckets[place] = BUCKET_WAS_FULL;
    ht->keys[place] = VALUE_NIL;
    return true;
  }
  return false;
}

// TODO: This function is almost carbon copy of ht_set2. Maybe there's a
// way not to duplicate code but I do not want to do any checks at runtime.
void ht_set(HashTable* ht, Value key)
{
  uint64 place = query(ht->buckets, ht->keys, ht->alloc_size, key);
  if (ht->buckets[place] == BUCKET_FULL) return;

  // So, hash table size will increase by one.
  ht->size++;

  // If map has room in it, simply add it.
  if (ht->size * 2 < ht->alloc_size){
    ht->buckets[place] = BUCKET_FULL;
    ht->keys[place] = key;
    return;
  }

  // Otherwise, expand the table and rehash
  rehash(ht, false);

  // Find a place again, and put the key in
  place = query(ht->buckets, ht->keys, ht->alloc_size, key);
  ht->buckets[place] = BUCKET_FULL;
  ht->keys[place] = key;
}

void ht_set2(HashTable* ht, Value key, Value value)
{
  uint64 place = query(ht->buckets, ht->keys, ht->alloc_size, key);
  if (ht->buckets[place] == BUCKET_FULL) {
    ht->values[place] = value;
    return;
  }

  // So, hash table size will increase by one.
  ht->size++;

  // If map has room in it, simply add it.
  if (ht->size * 2 < ht->alloc_size){
    ht->buckets[place] = BUCKET_FULL;
    ht->keys[place] = key;
    ht->values[place] = value;
    return;
  }

  // Otherwise, expand the table and rehash
  rehash(ht, true);

  // Find a place again, and put the key in
  place = query(ht->buckets, ht->keys, ht->alloc_size, key);
  ht->buckets[place] = BUCKET_FULL;
  ht->keys[place] = key;
  ht->values[place] = value;
}

#define INIT_SIZE 7
void ht_init(HashTable* ht)
{
  ht->size = 0;
  ht->alloc_size = INIT_SIZE;
  ht->buckets = mem_malloc(INIT_SIZE*sizeof(BucketType));
  ht->keys = mem_malloc(INIT_SIZE*sizeof(Value));
  ht->values = NULL;
}

void ht_clear(HashTable* ht)
{
  mem_free(ht->buckets);
  mem_free(ht->keys);
  ht_init(ht);
}

void ht_init2(HashTable* ht)
{
  ht->size = 0;
  ht->alloc_size = INIT_SIZE;
  ht->buckets = mem_malloc(INIT_SIZE*sizeof(BucketType));
  ht->keys = mem_malloc(INIT_SIZE*sizeof(Value));
  ht->values = mem_malloc(INIT_SIZE*sizeof(Value));
}
