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

#include "ripe/ripe.h"

const char* util_trim_ends(const char* input)
{
  char* txt = mem_strdup(input);
  txt++;
  txt[strlen(txt)-1] = 0;
  return txt;
}

const char* util_replace(const char* str, const char c, const char* replace)
{
  // Ultra inneficient, but who cares.
  StringBuf sb;
  sbuf_init(&sb, "");
  while(*str != 0){
    if (*str == c){
      sbuf_printf(&sb, "%s", replace);
    } else {
      sbuf_printf(&sb, "%c", *str);
    }
    str++;
  }
  const char* out = mem_strdup(sb.str);
  sbuf_deinit(&sb);
  return out;
}

const char* util_make_c_name(const char* ripe_name)
{
  return util_c_name(ripe_name);
}
