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

#include "clib/clib.h"

#define is_white(c)  ( (c) == ' ' or (c) == '\t' or (c) == '\n' )

void tok_init_white(Tok* tok, const char* str)
{
  tok_init(tok, str, " \n\t");
}

void tok_init(Tok* tok, const char* str, const char* delim)
{
  char* dup = mem_strdup(str);
  char* saveptr;

  char* token = strtok_r(dup, delim, &saveptr);
  if (token == NULL){
    tok->num = 0;
    tok->words = NULL;
    return;
  }
  tok->num = 1;
  tok->words = (const char**) mem_malloc(sizeof(char*));
  tok->words[0] = token;

  for(;;){
    token = strtok_r(NULL, delim, &saveptr);
    if (token == NULL) return;
    tok->num++;
    tok->words = (const char**) mem_realloc(tok->words, sizeof(char*) * tok->num);
    tok->words[tok->num-1] = token;
  }
}

void tok_destroy(Tok* tok)
{
  if (tok->num == 0) return;
  mem_free(tok->words);
}
