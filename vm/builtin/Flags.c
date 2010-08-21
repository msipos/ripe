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

Klass* klass_flag;

static Value flag_to_string(Value obj)
{
  switch(obj){
    case VALUE_NIL:
      return string_to_val("nil");
      break;
    case VALUE_FALSE:
      return string_to_val("false");
      break;
    case VALUE_TRUE:
      return string_to_val("true");
      break;
    case VALUE_EOF:
      return string_to_val("eof");
      break;
  }
  return VALUE_NIL;
}

void init1_Flags()
{
  klass_flag = klass_new(dsym_get("Flag"),
                         dsym_get("Object"),
                         KLASS_DIRECT,
                         0);
  klass_new_method(klass_flag,
                   dsym_get("to_string"),
                   func1_to_val(flag_to_string));
}

void init2_Flags()
{
}
