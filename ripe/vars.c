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

bool initialized = false;
static Array locals_arr; // A dictionary for each function (block).

static void locals_init()
{
  assert(initialized == false);

  initialized = true;
  array_init(&locals_arr, Dict*);
}

void push_locals()
{
  if (not initialized) locals_init();
  Dict* locals = dict_new(sizeof(char*), sizeof(Variable*),
                          dict_hash_string, dict_equal_string);
  array_append(&locals_arr, locals);
}

void pop_locals()
{
  if (not initialized) locals_init();
  array_pop(&locals_arr, Dict*);
}

void set_local(const char* ripe_name, const char* c_name, const char* type)
{
  assert(initialized);
  assert(locals_arr.size > 0);

  Variable* var = mem_new(Variable);
  var->c_name = c_name;
  var->ripe_name = ripe_name;
  var->type = type;
  Dict* dict = array_get(&locals_arr, Dict*, locals_arr.size - 1);
  dict_set(dict, &ripe_name, &var);
}

const char* register_local(const char* ripe_name, const char* type)
{
  assert(initialized);
  assert(locals_arr.size > 0);

  const char* c_name = util_make_c_name(ripe_name);
  set_local(ripe_name, c_name, type);
  return c_name;
}

// Test if ripe_name is a currently defined variable. Otherwise it should be
// treated like a static symbol.
Variable* query_local_full(const char* ripe_name)
{
  assert(initialized);
  assert(locals_arr.size > 0);

  for (int i = locals_arr.size - 1; i >= 0; i--){
    Dict* dict = array_get(&locals_arr, Dict*, i);
    Variable* var;
    if (dict_query(dict, &ripe_name, &var)){
      return var;
    }
  }
  return NULL;
}

const char* query_local(const char* ripe_name)
{
  assert(initialized);
  assert(locals_arr.size > 0);

  Variable* var = query_local_full(ripe_name);
  if (var == NULL) return NULL;
  return var->c_name;
}
