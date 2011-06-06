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

#include "lang/lang.h"

void input_from_file(RipeInput* input, const char* filename)
{
  input->filename = mem_strdup(filename);

  #define LINE_SIZE (10*1024)
  char line_buf[LINE_SIZE];

  // Populate the lines
  array_init(&(input->lines), char*);

  FILE* f = fopen(filename, "r");
  if (f == NULL){
    fatal_throw("cannot open '%s' for reading: %s", filename,
                strerror(errno));
  }

  for (;;){
    if (fgets(line_buf, LINE_SIZE, f) == NULL) break;
    char* s = mem_strdup(line_buf);
    array_append2(&(input->lines), &s);
  }
  fclose(f);
}

void input_from_string(RipeInput* input, const char* filename, const char* str)
{
  input->filename = filename;
  array_init(&(input->lines), char*);
  char* s = mem_strdup(str);
  array_append2(&(input->lines), &s);
}
