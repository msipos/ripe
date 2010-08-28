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

static Value ripe_to_string(Value self)
{
  char buf[128];
  sprintf(buf, "%f", unpack_double(self));
  return string_to_val(buf);
}

static Value ripe_is_double(Value v)
{
  if (is_double(v)) return VALUE_TRUE;
  return VALUE_FALSE;
}

static Value ripe_to_double(Value v)
{
  return double_to_val(val_to_double_soft(v));
}

double val_to_double_soft(Value v)
{
  if (is_int64(v)) return (double) unpack_int64(v);
  obj_verify(v, klass_Double);
  return unpack_double(v);
}

void init1_Double()
{
  klass_Double = klass_new(dsym_get("Double"),
                           dsym_get("Object"),
                           KLASS_DIRECT,
                           0);
  klass_new_method(klass_Double,
                   dsym_get("to_string"),
                   func1_to_val(ripe_to_string));

  ssym_set("is_Double?", func1_to_val(ripe_is_double));
  ssym_set("to_double", func1_to_val(ripe_to_double));
}

void init2_Double()
{
}
