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

///////////////////////////////////////////////////////////////////////////////
// Array1
///////////////////////////////////////////////////////////////////////////////

Array1* val_to_array1(Value v_array)
{
  obj_verify(v_array, klass_Array1);
  return obj_c_data(v_array);
}

Value array1_to_val2(uint16 num_args, ...)
{
  va_list ap;
  va_start(ap, num_args);
  Value* args = alloca(sizeof(Value) * num_args);
  for (uint16 i = 0; i < num_args; i++){
    args[i] = va_arg(ap, Value);
  }
  va_end(ap);

  return array1_to_val(num_args, args);
}

Value array1_to_val(int64 num_elements, Value* data)
{
  Array1* array;
  Value v = obj_new(klass_Array1, (void**) &array);
  array->size = num_elements;
  array->alloc_size = num_elements * 2;
  array->data = mem_malloc(sizeof(Value) * array->alloc_size);
  for (int64 i = 0; i < num_elements; i++){
    array->data[i] = data[i];
  }
  return v;
}

Value array1_new(int64 num_elements)
{
  Array1* array;
  Value v = obj_new(klass_Array1, (void**) &array);
  array->size = num_elements;
  array->alloc_size = num_elements * 2;
  array->data = mem_malloc(sizeof(Value) * num_elements);
  return v;
}

Value array1_index(Array1* array1, int64 idx)
{
  return array1->data[util_index("Array1", idx, array1->size)];
}

void array1_index_set(Array1* array1, int64 idx, Value val)
{
  array1->data[util_index("Array1", idx, array1->size)] = val;
}

Value array1_pop(Array1* a)
{
  int64 size = a->size - 1;
  Value rv = a->data[size];
  if (size*4 < a->alloc_size){
    a->alloc_size /= 2;
    if (a->alloc_size < 2) a->alloc_size = 2;
    a->data = mem_realloc(a->data, sizeof(Value) * a->alloc_size);
  }
  a->size = size;
  return rv;
}

void array1_push(Array1* a, Value val)
{
  int64 size = a->size + 1;
  a->size = size;
  if (size > a->alloc_size) {
    a->alloc_size *= 2;
    if (a->alloc_size == 0) {
      a->alloc_size = 2;
      a->data = mem_malloc(sizeof(Value) * a->alloc_size);
    } else {
      a->data = mem_realloc(a->data, sizeof(Value) * a->alloc_size);
    }
  }
  a->data[size - 1] = val;
}

///////////////////////////////////////////////////////////////////////////////
// Array2
///////////////////////////////////////////////////////////////////////////////

Klass* klass_Array2;

static Value ripe_array2_new_const(Value v_x, Value v_y, Value v_val)
{
  Array2* self;
  Value v_self = obj_new(klass_Array2, (void**) &self);

  int64 size_x = val_to_int64(v_x);
  int64 size_y = val_to_int64(v_y);

  self->size_x = size_x;
  self->size_y = size_y;
  if (size_x < 1 or size_y < 1)
    exc_raise("invalid Array3 size (%"PRId64"x%"PRId64")", size_x, size_y);
  int64 total_size = size_x * size_y;
  Value* data = mem_malloc(total_size * sizeof(Value));
  for (int64 t = 0; t < total_size; t++){
    data[t] = v_val;
  }
  self->data = data;
  return v_self;
}

static inline int64 array2_map_index(Array2* array, int64 x, int64 y)
{
  int64 size_x = array->size_x;
  int64 size_y = array->size_y;

  if (x > 0 and x <= size_x and y > 0 and y <= size_y){
    return (y - 1) + (x - 1) * size_y;
  } else {
    exc_raise("invalid Array2 index (%"PRId64", %"PRId64")", x, y);
  }
}

Value array2_index(Value v_array, int64 x, int64 y)
{
  obj_verify(v_array, klass_Array2);
  Array2* array = obj_c_data(v_array);
  return array->data[array2_map_index(array, x, y)];
}

void array2_index_set(Value v_array, int64 x, int64 y, Value v_val)
{
  obj_verify(v_array, klass_Array2);
  Array2* array = obj_c_data(v_array);
  array->data[array2_map_index(array, x, y)] = v_val;
}

static Value ripe_array2_index(Value v_self, Value v_x, Value v_y)
{
  Array2* array = obj_c_data(v_self);
  return array->data[array2_map_index(array,
                                      val_to_int64(v_x),
                                      val_to_int64(v_y))];
}

static Value ripe_array2_index_set(Value v_self, Value v_x, Value v_y,
                                   Value v_val)
{
  Array2* array2 = obj_c_data(v_self);
  array2->data[array2_map_index(array2,
                                val_to_int64(v_x),
                                val_to_int64(v_y))] = v_val;
  return VALUE_NIL;
}

static Value ripe_array2_get_size_x(Value v_self)
{
  Array2* array = obj_c_data(v_self);
  return int64_to_val(array->size_x);
}

static Value ripe_array2_get_size_y(Value v_self)
{
  Array2* array = obj_c_data(v_self);
  return int64_to_val(array->size_y);
}

static Value ripe_array2_set_const(Value v_self, Value v_val)
{
  Array2* array = obj_c_data(v_self);
  int64 total_size = array->size_x * array->size_y;
  for (int64 i = 0; i < total_size; i++){
    array->data[i] = v_val;
  }
  return VALUE_NIL;
}

///////////////////////////////////////////////////////////////////////////////
// Array3
///////////////////////////////////////////////////////////////////////////////
uint64 array3_index(Array3* array, uint64 x, uint64 y, uint64 z)
{
  uint64 size_x = array->size_x;
  uint64 size_y = array->size_y;
  uint64 size_z = array->size_z;

  if (x > 0 and x <= size_x and y > 0 and y <= size_y
                            and z > 0 and z <= size_z){
    // C order
    return (x-1) + (y-1)*size_x + (z-1)*size_x*size_y;
  } else {
    exc_raise("invalid Array3 index (%"PRIu64", %"PRIu64", %"PRIu64")",
              x, y, z);
  }
}


///////////////////////////////////////////////////////////////////////////////
// Init
///////////////////////////////////////////////////////////////////////////////

void init1_Arrays()
{
  klass_Array2 = klass_new(dsym_get("Array2"),
                           dsym_get("Object"),
                           KLASS_CDATA_OBJECT,
                           sizeof(Array2));

  // Array2
  ssym_set("Array2.new_const", func3_to_val(ripe_array2_new_const));
  klass_new_method(klass_Array2,
                   dsym_get("index"),
                   func3_to_val(ripe_array2_index));
  klass_new_method(klass_Array2,
                   dsym_get("index_set"),
                   func4_to_val(ripe_array2_index_set));
  klass_new_method(klass_Array2,
                   dsym_get("get_size_x"),
                   func1_to_val(ripe_array2_get_size_x));
  klass_new_method(klass_Array2,
                   dsym_get("get_size_y"),
                   func1_to_val(ripe_array2_get_size_y));
  klass_new_method(klass_Array2,
                   dsym_get("set_const"),
                   func2_to_val(ripe_array2_set_const));
}

void init2_Arrays()
{

}
