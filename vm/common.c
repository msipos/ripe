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

Klass* klass_Array1;
Klass* klass_Array3;
Klass* klass_Bool;
Klass* klass_Destroyed;
Klass* klass_Double;
Klass* klass_Eof;
Klass* klass_Error;
Klass* klass_Function;
Klass* klass_Integer;
Klass* klass_Nil;
Klass* klass_Map;
Klass* klass_Range;
Klass* klass_Set;
Klass* klass_String;
Klass* klass_Tuple;

void common_init_phase15()
{
  klass_Array1 = klass_get(dsym_get("Array1"));
  klass_Array3 = klass_get(dsym_get("Array3"));
  klass_Bool = klass_get(dsym_get("Bool"));
  klass_Destroyed = klass_get(dsym_get("Destroyed"));
  klass_Double = klass_get(dsym_get("Double"));
  klass_Error = klass_get(dsym_get("Error"));
  klass_Eof = klass_get(dsym_get("Eof"));
  klass_Function = klass_get(dsym_get("Function"));
  klass_Integer = klass_get(dsym_get("Integer"));
  klass_Map = klass_get(dsym_get("Map"));
  klass_Nil = klass_get(dsym_get("Nil"));
  klass_Range = klass_get(dsym_get("Range"));
  klass_Set = klass_get(dsym_get("Set"));
  klass_String = klass_get(dsym_get("String"));
  klass_Tuple = klass_get(dsym_get("Tuple"));
}
