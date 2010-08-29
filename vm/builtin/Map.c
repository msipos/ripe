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
#include "clib/stringbuf.h"

Klass* klass_Map;

static void set(Bucket* buckets, int64 num_buckets, Value key, Value value)
{
  uint64 h = op_hash(key);
  for (int64 idx = 0; idx < num_buckets; idx++){
    // Quadratic probing
    int64 place = (h + idx*idx) % num_buckets;

    if (buckets[place].type != BUCKET_FULL){
      buckets[place].type = BUCKET_FULL;
      buckets[place].key = key;
      buckets[place].value = value;
      return;
    }
    if (op_equal2(key, buckets[place].key)){
      buckets[place].value = value;
      return;
    }
  }
  assert_never();
}

bool map_query(Map* map, Value key, Value* value)
{
  uint64 h = op_hash(key);
  int64 alloc_size = map->alloc_size;
  Bucket* buckets = map->buckets;
  for (int64 idx = 0; idx < alloc_size; idx++){
    int64 place = (h + idx*idx) % alloc_size;

    switch(buckets[place].type){
      case BUCKET_EMPTY:
        return false;
      case BUCKET_FULL:
        if (op_equal2(buckets[place].key, key)){
          if (value != NULL){
            *value = buckets[place].value;
          }
          return true;
        }
        break;
      default:
        return false;
    }
  }
  assert_never();
}

void map_set(Map* map, Value key, Value value)
{
  // If key already exist in map, then simply replace it.
  if (map_query(map, key, NULL)){
    set(map->buckets, map->alloc_size, key, value);
    return;
  }

  // So, map size will increase by one.
  map->size++;

  // If map still has room in it, simply add it.
  if (map->size * 2 < map->alloc_size){
    set(map->buckets, map->alloc_size, key, value);
    return;
  }

  // Otherwise, increase the size of the hash table and rehash..

  // Allocate new buckets
  int64 alloc_size = map->alloc_size;
  int64 new_alloc_size = map_prime(alloc_size); // map_prime is in
                                                     // clib/dict.c
  Bucket* buckets = map->buckets;
  Bucket* new_buckets = mem_calloc(new_alloc_size*sizeof(Bucket));

  // Iterate over old buckets and rehash
  for (int64 i = 0; i < alloc_size; i++){
    if (buckets[i].type == BUCKET_FULL){
      set(new_buckets, new_alloc_size, buckets[i].key, buckets[i].value);
    }
  }

  // Finally add new key
  set(new_buckets, new_alloc_size, key, value);

  // Release old buckets and replace them
  mem_free(map->buckets);
  map->buckets = new_buckets;
  map->alloc_size = new_alloc_size;
}

void map_init(Map* map)
{
  map->size = 0;
  map->alloc_size = map_prime(1);
  map->buckets = mem_calloc(map->alloc_size * sizeof(Bucket));
}

static Value ripe_map_new()
{
  Map* map;
  Value __self = obj_new(klass_Map, (void**) &map);
  map_init(map);
  return __self;
}

static Value ripe_map_index(Value __self, Value __key)
{
  Map* map = obj_c_data(__self);
  Value result;
  if (map_query(map, __key, &result)){
    return result;
  }
  exc_raise("key error");
}

static Value ripe_map_index_set(Value __self, Value __key, Value __value)
{
  Map* map = obj_c_data(__self);
  map_set(map, __key, __value);
  return VALUE_NIL;
}

static Value ripe_map_to_string(Value __self)
{
  StringBuf sb;
  sbuf_init(&sb, "");
  sbuf_printf(&sb, "Map (");
  Map* map = obj_c_data(__self);
  int64 alloc_size = map->alloc_size;
  Bucket* buckets = map->buckets;
  bool passed_first = false;
  for (int64 i = 0; i < alloc_size; i++){
    if (buckets[i].type == BUCKET_FULL){
      if (passed_first){
        sbuf_printf(&sb, ", ");
      }
      sbuf_printf(&sb, "%s => %s", to_string(buckets[i].key), to_string(buckets[i].value));
      passed_first = true;
    }
  }
  sbuf_printf(&sb, ")");
  Value rv = string_to_val(sb.str);
  sbuf_deinit(&sb);
  return rv;
}

static Value ripe_map_get_size(Value __self)
{
  Map* map = obj_c_data(__self);
  return int64_to_val(map->size);
}

static Value ripe_map_has_key(Value __self, Value __key)
{
  Map* map = obj_c_data(__self);
  return pack_bool(map_query(map, __key, NULL));
}

void init1_Map()
{
  klass_Map = klass_new(dsym_get("Map"),
                        dsym_get("Object"),
                        KLASS_OBJECT,
                        sizeof(Map));
  ssym_set("Map.new", func0_to_val(ripe_map_new));
  klass_new_method(klass_Map,
                   dsym_get("index"),
                   func2_to_val(ripe_map_index));
  klass_new_method(klass_Map,
                   dsym_get("index_set"),
                   func3_to_val(ripe_map_index_set));
  klass_new_method(klass_Map,
                   dsym_get("to_string"),
                   func1_to_val(ripe_map_to_string));
  Value __map_get_size = func1_to_val(ripe_map_get_size);
  klass_new_method(klass_Map,
                   dsym_get("get_size"),
                   __map_get_size);
  klass_new_virtual_reader(klass_Map,
                           dsym_get("size"),
                           __map_get_size);
  klass_new_method(klass_Map,
                   dsym_get("has_key?"),
                   func2_to_val(ripe_map_has_key));
}

void init2_Map()
{
}
