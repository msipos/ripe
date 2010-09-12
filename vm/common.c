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
Klass* klass_Range;
Klass* klass_String;
Klass* klass_Tuple;

Dsym dsym_to_string;

static uint64 param(char* out, Value v)
{
  if (is_int64(v)){
    char buf[200];
    sprintf(buf, "%"PRId64, unpack_int64(v));
    if (out) strcpy(out, buf);
    return strlen(buf) + 1;
  }

  if (is_double(v)){
    char buf[200];
    sprintf(buf, "%g", unpack_double(v));
    if (out) strcpy(out, buf);
    return strlen(buf) + 1;
  }

  Klass* k = obj_klass(v);
  if (k == klass_String){
    char** s = obj_c_data(v);
    if (out) strcpy(out, *s);
    return strlen(*s) + 1;
  }

  const char* buf = to_string(v);
  if (out) strcpy(out, buf);
  return strlen(buf) + 1;
}

uint64 common_simple_format(char* out, uint64 num_values, Value* values)
{
  uint64 i = 0;
  for (uint64 idx = 0; idx < num_values; idx++){
    Value v = values[idx];

    char p[param(NULL, v)];
    param(p, v);
    if (out) strcpy(out + i, p);
    i += strlen(p);
  }
  if (out) out[i] = 0;
  i++;
  return i;
}

uint64 common_format(char* out, char* format_string, uint64 num_values, Value* values)
{
  uint64 i = 0;
  char* s = format_string;

  bool inparam = false;
  uint64 n = 0;
  while (*s != 0){
    if (*s == '%'){
      if (inparam){
        inparam = false;
        if (n >= num_values) exc_raise("not enough format parameters");

        // Now, convert value to string
        Value v = values[n];
        n++;
        char p[param(NULL, v)];
        param(p, v);

        // Update i, and optionally print
        if (out) strcpy(out + i, p);
        i += strlen(p);
      } else {
        inparam = true;
      }
    } else {
      if (inparam) {
        // consume
      } else {
        if (out) out[i] = *s;
        i++;
      }
    }
    s++;
  }

  if (out) out[i] = 0;
  i++;
  return i;
}

void common_init_phase15()
{
  klass_Nil = klass_get(dsym_get("Nil"));
  klass_False = klass_get(dsym_get("False"));
  klass_True = klass_get(dsym_get("True"));
  klass_Eof = klass_get(dsym_get("Eof"));
  klass_Integer = klass_get(dsym_get("Integer"));
  klass_Double = klass_get(dsym_get("Double"));
  klass_Array1 = klass_get(dsym_get("Array1"));
  klass_Range = klass_get(dsym_get("Range"));
  klass_String = klass_get(dsym_get("String"));
  klass_Tuple = klass_get(dsym_get("Tuple"));

  dsym_to_string = dsym_get("to_string");
}
