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

#include <ctype.h>
#include "vm/vm.h"

// Strip whitespace in place.
static void strip_whitespace(char* s)
{
  // Go backwards and remove whitespace
  int l = strlen(s);
  for(;;){
    if (l == 0) break;
    char c = s[l-1];
    if (c == ' '){
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
    if (c == ' '){
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

static const char* param_to_string(Value v)
{
  switch (v & MASK_TAIL){
    case 0b00:
      if (v == VALUE_NIL) return "nil";
      if (v == VALUE_TRUE) return "true";
      if (v == VALUE_FALSE) return "false";
      if (v == VALUE_EOF) return "eof";
      return to_string(v);
    case 0b01:
      {
        char buf[30];
        sprintf(buf, "%"PRId64, unpack_int64(v));
        return mem_strdup(buf);
      }
    case 0b10:
      {
        char buf[30];
        sprintf(buf, "%g", unpack_double(v));
        return mem_strdup(buf);
      }
    case 0b11:
      assert_never();
  }
  assert_never();
  return NULL;
}

#define FORMAT_REPORT_UNUSED

char* format_to_string(const char* fstr, uint64 num_values, Value* values)
{
  FormatParse fp;
  if (format_parse(fstr, &fp)){
    exc_raise("failed to parse format string '%s'", fstr);
  }


  const char* strings[num_values];
  for (uint64 i = 0; i < num_values; i++){
    strings[i] = NULL;
  }

  #ifdef FORMAT_REPORT_UNUSED
  bool param_used[num_values];
  for (uint64 i = 0; i < num_values; i++) param_used[i] = false;
  #endif

  uint64 out_len = 1; // 1 for NULL
  for (uint64 i = 0; i < fp.size; i++){
    switch(fp.elements[i].type) {
      case FORMAT_STRING:
        out_len += strlen(fp.elements[i].str);
        break;
      case FORMAT_NUMBER:
        {
          int64 n = fp.elements[i].number-1;

          if (n < 0){
            exc_raise("0 or negative argument to format '%s'", fstr);
          }
          if (n >= num_values){
            exc_raise("not enough arguments to format '%s' (looked for "
                      "argument %"PRId64" but have %"PRId64" arguments)",
                      fstr, n+1, num_values);
          }

          #ifdef FORMAT_REPORT_UNUSED
          param_used[n] = true;
          #endif

          if (strings[n] == NULL) {
            strings[n] = param_to_string(values[n]);
          }
          out_len += strlen(strings[n]);
        }
        break;
      default:
        assert_never();
    }
  }

  #ifdef FORMAT_REPORT_UNUSED
  for (uint64 i = 0; i < num_values; i++){
    if (not param_used[i]){
      char* tmp_fstr = mem_strdup(fstr);
      strip_whitespace(tmp_fstr);
      fprintf(stderr,
              "WARNING: argument %"PRIu64" ('%s') not used in "
              "format string '%s'\n",
              i+1, param_to_string(values[i]), tmp_fstr);
    }
  }
  #endif

  char out[out_len];
  out_len = 0;
  for (uint64 i = 0; i < fp.size; i++){
    switch(fp.elements[i].type) {
      case FORMAT_STRING:
        strcpy(out + out_len, fp.elements[i].str);
        out_len += strlen(fp.elements[i].str);
        break;
      case FORMAT_NUMBER:
        {
          int64 n = fp.elements[i].number-1;
          strcpy(out + out_len, strings[n]);
          out_len += strlen(strings[n]);
        }
        break;
    }
  }
  return mem_strdup(out);
}

static char scan_until(const char** s, char terminate)
{
  for(;;){
    if (**s == 0) return 0;
    if (**s == terminate) return terminate;
    (*s)++;
  }
}

static void add_element(FormatParse* fp)
{
  if (fp->size == 0){
    fp->size = 1;
    fp->elements = mem_new(FormatParseElement);
  } else {
    fp->size++;
    fp->elements = mem_realloc(fp->elements,
                               sizeof(FormatParseElement) * fp->size);
  }
}

int format_parse(const char* fstr, FormatParse* fp)
{
  fp->size = 0;
  fp->elements = NULL;

  const char* cur = fstr;
  uint64 element = 0;
  int64 autocounter = 1;
  for(;;){
    if (*cur == 0){
      break;
    }
    if (*cur == '{') {
      char* str = (char*) cur;
      char c = scan_until(&cur, '}');
      if (c == 0) return 1; // Error
      // Now, *cur = '}', and *str = '{'

      // Copy over to param_str
      uint64 tmp_len = cur - str;
      char param_str[tmp_len];
      strncpy(param_str, str+1, tmp_len - 1);
      param_str[tmp_len-1] = 0;
      cur++;

      // Process param_str
      strip_whitespace(param_str);

      // FORMAT_NUMBER
      if (isdigit(param_str[0])){
        char* param_end;
        int64 n = strtol(param_str, &param_end, 0);
        if (n == 0 or *param_end != 0){
          return 1;
        }

        // Add it
        add_element(fp);
        fp->elements[element].type = FORMAT_NUMBER;
        fp->elements[element].number = n;
      } else if (param_str[0] == '\0') {
        add_element(fp);
        fp->elements[element].type = FORMAT_NUMBER;
        fp->elements[element].number = autocounter;
        autocounter++;
      } else if (param_str[0] == '{') {
        add_element(fp);
        fp->elements[element].type = FORMAT_STRING;
        fp->elements[element].str = "{";
      } else if (param_str[0] == '}') {
        add_element(fp);
        fp->elements[element].type = FORMAT_STRING;
        fp->elements[element].str = "}";
      } else {
        return 1;
      }

    } else {
      char* str = (char*) cur;
      char c = scan_until(&cur, '{');
      str = mem_strndup(str, cur - str + 1);
      add_element(fp);
      fp->elements[element].type = FORMAT_STRING;
      fp->elements[element].str = str;
    }
    element++;
  }
  return 0;
}
