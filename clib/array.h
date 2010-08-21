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

#ifndef ARRAY_H
#define ARRAY_H

#include "clib.h"

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

#define array_get(arr, T, i)        (((T *) (arr)->data)[i])
#define array_append(arr, value)  { array_expand(arr, sizeof(value)); \
                                    ((typeof(value) *) (arr)->data)[(arr)->size] = value; \
                                    (arr)->size++; }
#define array_pop(arr, T)         ({(arr)->size--; (((T*) (arr)->data)[(arr)->size]); })
void array_prepend(Array* arr, uint sz, void* el);
void array_expand(Array* arr, uint sz);
void array_clear(Array* arr);

#endif
