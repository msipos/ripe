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

typedef struct {
  char* str;
} String;

Klass* klass_string;

static Value string_to_string(Value self)
{
  return self;
}

static Value ripe_string_hash(Value __self)
{
  String* string = obj_c_data(__self);
  return pack_int64((int64) hash_bytes((uint8*)string->str, strlen(string->str)+1, 43));
}

void init1_String()
{
  klass_string = klass_new(dsym_get("String"),
                           dsym_get("Object"),
                           KLASS_CDATA_OBJECT,
                           sizeof(String));
  klass_new_method(klass_string,
                   dsym_get("to_string"), 
                   func1_to_val(string_to_string));
  klass_new_method(klass_string,
                   dsym_get("hash"), 
                   func1_to_val(ripe_string_hash));
}

void init2_String()
{
}

char* val_to_string(Value v)
{
  obj_verify(v, klass_string);
  String* obj = obj_c_data(v);
  return obj->str;
}

Value string_to_val(char* str)
{
  String* obj;
  Value v = obj_new(klass_string, (void**)&obj);
  obj->str = mem_strdup(str);
  return v;
}

