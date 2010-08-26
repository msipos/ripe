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

Klass* klass_integer;

static Value integer_to_string(Value self)
{
  char buf[128];
  sprintf(buf, "%"PRId64, unpack_int64(self));
  return string_to_val(buf);
}

void init1_Integer()
{
  klass_integer = klass_new(dsym_get("Integer"),
                         dsym_get("Object"),
                         KLASS_DIRECT,
                         0);
  klass_new_method(klass_integer,
                   dsym_get("to_string"),
                   func1_to_val(integer_to_string));
}

void init2_Integer()
{
}

int64 val_to_int64(Value v)
{
  obj_verify(v, klass_integer);
  return unpack_int64(v);
}

int64 val_to_int64_soft(Value v)
{
  if (is_int64(v)) return unpack_int64(v);
  if (is_double(v)) return (int64) val_to_double(v);
  exc_raise("Value not an Integer or Double (%s given)", 
            dsym_reverse_get(klass_get(v)->name));
}

