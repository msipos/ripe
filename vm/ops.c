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

int64 op_hash(Value v)
{
  switch(v & MASK_TAIL){
    case 0b00:
      {
        Klass* k = obj_klass(v);
        if (k == klass_String) {
          String* string = obj_c_data(v);
          return hash_string(string->str);
        } else if (k == klass_Tuple) {
          uint64 h = 89;
          Tuple* tuple = obj_c_data(v);
          int64 size = tuple->size;
          for (int64 i = 0; i < size; i++){
            h += op_hash(tuple->data[i]);
          }
          return h;
        }
        return hash_value(v);
      }
    case 0b01:
    case 0b10:
    case 0b11:
      return unpack_int64(v);
  }
  assert_never();

  // Ignore warning:
  return 0;
}

Value op_and(Value a, Value b)
{
  return pack_bool((a == VALUE_TRUE) and (b == VALUE_TRUE));
}

Value op_or(Value a, Value b)
{
  return pack_bool((a == VALUE_TRUE) or (b == VALUE_TRUE));
}

static inline bool op_equal3(Value a, Value b)
{
  if (obj_klass(a) == klass_String
       and
      obj_klass(b) == klass_String){
    return strcmp(val_to_string(a), val_to_string(b)) == 0;
  }
  if (obj_klass(a) == klass_Tuple
       and
      obj_klass(b) == klass_Tuple){
    Tuple* ta = obj_c_data(a);
    Tuple* tb = obj_c_data(b);
    if (ta->size != tb->size) return false;
    int64 sz = ta->size;
    for (int64 i = 0; i < sz; i++){
      if (not op_equal3(ta->data[i], tb->data[i])){
        return false;
      }
    }
    return true;
  }
  return a == b;
}

Value op_equal(Value a, Value b)
{
  return pack_bool(op_equal3(a, b));
}

bool op_equal2(Value a, Value b)
{
  return op_equal3(a, b);
}

Value op_not_equal(Value a, Value b)
{
  return pack_bool(not op_equal3(a, b));
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
