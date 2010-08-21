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

Value op_and(Value a, Value b)
{
  return pack_bool((a == VALUE_TRUE) and (b == VALUE_TRUE));
}

Value op_or(Value a, Value b)
{
  return pack_bool((a == VALUE_TRUE) or (b == VALUE_TRUE));
}

Value op_equal(Value a, Value b)
{
  if (obj_klass(a) == klass_string 
       and
      obj_klass(b) == klass_string){
    return pack_bool(strcmp(val_to_string(a), val_to_string(b)) == 0);
  }
  return pack_bool(a == b);
}

bool op_equal2(Value a, Value b)
{
  if (obj_klass(a) == klass_string 
       and
      obj_klass(b) == klass_string){
    return strcmp(val_to_string(a), val_to_string(b)) == 0;
  }
  return a == b;
}


Value op_not_equal(Value a, Value b)
{
  return pack_bool(a != b);
}

Value op_unary_not(Value v)
{
  switch(v){
    case VALUE_TRUE:
      return VALUE_FALSE;
    case VALUE_FALSE:
      return VALUE_TRUE;
  }
  exc_raise("unary 'not' called with non-bool value");
}

Value op_unary_minus(Value v)
{
  switch(v & MASK_TAIL){
    case 0b01:
      return pack_int64(-unpack_int64(v));
    case 0b10:
      return pack_double(-unpack_double(v));
  }
  exc_raise("unary '-' called with non-numerical value");
}
