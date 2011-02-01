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

static Klass* klass_object;

static Value ripe_to_string(Value obj)
{
  char buf[128];
  sprintf(buf, "<%s,%"PRIu64">", dsym_reverse_get(obj_klass(obj)->name), obj);
  //return string_to_val(buf);
  return VALUE_NIL;
}

void init1_Object(){
  klass_object = klass_new(dsym_get("Object"),
                           0);
  klass_new_method(klass_object,
                   dsym_get("to_string"),
                   func1_to_val(ripe_to_string));
}

void init2_Object(){
}
