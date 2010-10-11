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

int sys_argc;
char** sys_argv;

Klass* klass_Nil;
Klass* klass_False;
Klass* klass_True;
Klass* klass_Eof;
Klass* klass_Integer;
Klass* klass_Double;
Klass* klass_Array1;
Klass* klass_Array3;
Klass* klass_Range;
Klass* klass_String;
Klass* klass_Tuple;

Value dsym_to_string;

void common_init_phase15()
{
  klass_Nil = klass_get(dsym_get("Nil"));
  klass_False = klass_get(dsym_get("False"));
  klass_True = klass_get(dsym_get("True"));
  klass_Eof = klass_get(dsym_get("Eof"));
  klass_Integer = klass_get(dsym_get("Integer"));
  klass_Double = klass_get(dsym_get("Double"));
  klass_Array1 = klass_get(dsym_get("Array1"));
  klass_Array3 = klass_get(dsym_get("Array3"));
  klass_Range = klass_get(dsym_get("Range"));
  klass_String = klass_get(dsym_get("String"));
  klass_Tuple = klass_get(dsym_get("Tuple"));

  dsym_to_string = dsym_get("to_string");
}
