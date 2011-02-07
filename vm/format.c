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

// Return how many characters you consumed
static int64 scan(char* s, int* type)
{
  char c = *s;

  if (c == '%'){
    if (s[1] == '{'){
      *type = FORMAT_PARAM;
      int64 i = 2;
      for (;;){
        if (s[i] == '}'){
          i++;
          break;
        }
        if (s[i] == 0){
          break;
        }
        i++;
      }
      return i;
    } else {
      *type = FORMAT_FLAG;
      return 2;
    }
  } else {
    *type = FORMAT_STRING;
    int64 i = 0;
    for (;;){
      if (s[i] == '%' or s[i] == 0) {
        break;
      }
      i++;
    }
    return i;
  }
}

// Strip whitespace in place
static void strip_whitespace(char* s)
{
  // Go backwards and remove whitespace
  int l = strlen(s);
  for(;;){
    if (l == 0) break;
    char c = s[l-1];
    if (c == ' ' or c == '\t' or c== '\n'){
      s[l-1] = 0;
    } else {
      break;
    }
    l--;
  }

  // Go forwards and remove whitespace
  l = 0;
  for(;;){
    char c = s[l];
    if (c == 0) break;
    if (c == ' ' or c == '\t' or c== '\n'){
      l++;
    } else {
      break;
    }
  }

  int64 i = 0;
  for(;;){
    s[i] = s[i + l];
    if (s[i + l] == 0) break;
    i++;
  }
}

void format_get_text(Format* f, int64 i)
{
  FormatElement e = ((FormatElement*) f->array.data)[i];
  switch (e.type){
    case FORMAT_STRING:
      sbuf_ncpy(&(f->sb), f->string + e.a, e.b - e.a + 1);
      break;
    case FORMAT_FLAG:
      sbuf_ncpy(&(f->sb), f->string + e.a + 1, 1);
      break;
    case FORMAT_PARAM:
      sbuf_ncpy(&(f->sb), f->string + e.a + 2, e.b - e.a - 2);
      strip_whitespace(f->sb.str);
      break;
  }
}

void format_init(Format* f, char* string)
{
  f->string = string;
  array_init(&(f->array), FormatElement);
  sbuf_init(&(f->sb), "");

  int64 i = 0;
  for (;;){
    if (string[i] == 0) break;

    int64 start = i;
    FormatElement e;
    i += scan(string + i, &e.type);
    e.a = start;
    e.b = i - 1;
    array_append(&(f->array), e);
  }
}

void format_deinit(Format* f)
{
  array_deinit(&(f->array));
  sbuf_deinit(&(f->sb));
}

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

uint64 format_simple(char* out, uint64 num_values, Value* values)
{
  uint64 i = 0;
  for (uint64 idx = 0; idx < num_values; idx++){
    Value v = values[idx];

    uint64 sz = param(NULL, v);
    if (out) {
      char p[sz];
      param(p, v);
      strcpy(out + i, p);
    }
    i += sz - 1;
  }
  if (out) out[i] = 0;
  i++;
  return i;
}

uint64 format(char* out, char* format_string, uint64 num_values, Value* values)
{
  Format f;
  format_init(&f, format_string);
  uint64 sz = 0;
  uint64 p = 0;
  if (out) out[0] = 0;

  FormatElement* data = (FormatElement*) f.array.data;
  for (int64 i = 0; i < f.array.size; i++){
    FormatElement e = data[i];

    switch (e.type){
      case FORMAT_STRING:
        if (out) {
          format_get_text(&f, i);
          strcpy(out + sz, f.sb.str);
        }
        sz += e.b - e.a + 1;
        break;
      case FORMAT_FLAG:
        format_get_text(&f, i);
        char c = f.sb.str[0];
        switch (c){
          case '%':
            if (out) {
              out[sz] = '%';
              out[sz + 1] = 0;
            }
            sz++;
            break;
          case 'a':
            {
              if (p >= num_values) exc_raise("not enough parameters given");
              Value v = values[p];
              int64 tsz = param(NULL, v);
              if (out) {
                char buf[tsz];
                param(buf, v);
                strcpy(out + sz, buf);
              }
              sz += tsz - 1;
              p++;
            }
            break;
          default:
            exc_raise("invalid format flag '%%%c'", c);
        }
        break;
      default:
        exc_raise("invalid format string");
    }
  }
  format_deinit(&f);
  if (out) out[sz] = 0;
  return sz + 1;
}

void format_parse(char* fstr, FormatParse* fp)
{



}
