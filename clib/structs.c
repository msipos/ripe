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

#include "clib/clib.h"

void sarray_init(SArray* arr)
{
  arr->size = 0;
  arr->alloc_size = 2;
  arr->data = (SElement*) mem_malloc(2 * sizeof(SElement));
}

static void sarray_expand(SArray* arr)
{
  assert(arr != NULL);
  assert(arr->size >= 0);
  assert(arr->alloc_size > 0);

  if (arr->alloc_size > arr->size){
    return;
  }

  arr->alloc_size *= 2;
  arr->data = (SElement*) mem_realloc(arr->data, arr->alloc_size*sizeof(SElement));
}

void sarray_pop(SArray* arr)
{
  assert(arr != NULL); assert(arr->size > 0);
  arr->size--;
}

//            PTR INTERFACE

void sarray_append_ptr(SArray* arr, const void* v)
{
  sarray_expand(arr);
  arr->data[arr->size].p = (void*) v;
  arr->size++;
}

void* sarray_get_ptr(SArray* arr, int idx)
{
  assert(arr != NULL); assert(idx >= 0); assert(idx < arr->size);

  return arr->data[idx].p;
}

void* sarray_pop_ptr(SArray* arr)
{
  assert(arr != NULL); assert(arr->size > 0);
  arr->size--;
  return arr->data[arr->size].p;
}
