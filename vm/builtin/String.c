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

char* val_to_string(Value v)
{
  obj_verify(v, klass_String);
  String* obj = obj_c_data(v);
  return obj->str;
}

Value string_to_val(const char* str)
{
  assert(str != NULL);
  String* obj;
  Value v = obj_new(klass_String, (void**)&obj);
  obj->str = mem_strdup(str);
  obj->type = STRING_REGULAR;
  return v;
}

Value stringn_to_val(const char* str, int n)
{
  assert(str != NULL);
  String* obj;
  Value v = obj_new(klass_String, (void**)&obj);
  obj->str = mem_malloc(n+1);
  strncpy(obj->str, str, n);
  obj->str[n] = 0;
  obj->type = STRING_REGULAR;
  return v;
}

Value string_const_to_val(const char* str)
{
  assert(str != NULL);
  String* obj;
  Value v = obj_new(klass_String, (void**)&obj);
  obj->str = (char*) str;
  obj->type = STRING_CONST;
  return v;
}
