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

Tuple* val_to_tuple(Value v_tuple)
{
  obj_verify(v_tuple, klass_Tuple);
  return obj_c_data(v_tuple);
}

Value tuple_to_val(uint16 num_args, ...)
{
  va_list ap;
  va_start(ap, num_args);

  Tuple* tuple;
  Value v = obj_new(klass_Tuple, (void**) &tuple);
  tuple->size = num_args;
  tuple->data = mem_malloc(sizeof(Value)*num_args);
  for (uint i = 0; i < num_args; i++){
    tuple->data[i] = va_arg(ap, Value);
  }
  va_end(ap);
  return v;
}

Value tuple_index(Tuple* tuple, int64 idx)
{
  return tuple->data[util_index("Tuple", idx, tuple->size)];
}

void tuple_index_set(Tuple* tuple, int64 idx, Value val)
{
  tuple->data[util_index("Tuple", idx, tuple->size)] = val;
}
