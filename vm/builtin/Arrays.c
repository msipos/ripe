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

Value array1_to_val2(int num_args, ...)
{
  va_list ap;
  va_start(ap, num_args);

  Array1* array;
  Value v = obj_new(klass_Array1, (void**) &array);
  array->size = num_args;
  array->alloc_size = num_args;
  array->data = mem_malloc(sizeof(Value) * array->alloc_size);
  for (int i = 0; i < num_args; i++){
    array->data[i] = va_arg(ap, Value);
  }
  va_end(ap);

  return v;
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
  if (a->size == 0)
    exc_raise("pop() from an empty array");

  uint64 size = a->size - 1;
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
  uint64 size = a->size + 1;
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

int64 array2_index(Array2* array, int64 x, int64 y)
{
  int64 size_x = array->size_x;
  int64 size_y = array->size_y;

  if (x > 0 and x <= size_x and y > 0 and y <= size_y){
    return (y - 1) + (x - 1) * size_y;
  } else {
    exc_raise("invalid Array2 index (%"PRId64", %"PRId64")", x, y);
  }
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
