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

typedef struct {
  const char* c_name;
  const char* ripe_name;
  const char* type;
} Variable;
static Array locals_arr; // A dictionary for each function (block).

void var_init()
{
  array_init(&locals_arr, Dict*);
}

void var_push()
{
  Dict* locals = dict_new(sizeof(char*), sizeof(Variable*),
                          dict_hash_string, dict_equal_string);
  array_append(&locals_arr, locals);
}

void var_pop()
{
  assert(locals_arr.size > 0);
  array_pop(&locals_arr, Dict*);
}

void var_add_local(const char* ripe_name, const char* c_name, 
                   const char* type)
{
  assert(locals_arr.size > 0);

  if (var_query(ripe_name)){
    fatal_throw("variable '%s' already defined", ripe_name);
  }

  Variable* var = mem_new(Variable);
  var->c_name = c_name;
  var->ripe_name = ripe_name;
  var->type = type;
  Dict* dict = array_get(&locals_arr, Dict*, locals_arr.size - 1);
  dict_set(dict, &ripe_name, &var);
}

static Variable* query(const char* ripe_name)
{
  // First check local variables
  if (locals_arr.size > 0) {
    for (int i = locals_arr.size - 1; i >= 0; i--){
      Dict* dict = array_get(&locals_arr, Dict*, i);
      Variable* var;
      if (dict_query(dict, &ripe_name, &var)){
        return var;
      }
    }
  }
  
  // Now check globals
  GlobalInfo* gi = stran_query_global(ripe_name);
  if (gi == NULL) return NULL;
  
  // Write out the prototype
  cache_global_prototype(ripe_name);
  
  Variable* var = mem_new(Variable);
  var->ripe_name = ripe_name;
  var->c_name = gi->c_name;
  var->type = "?"; // TODO
  return var;
}

bool var_query(const char* ripe_name)
{
  return query(ripe_name) != NULL;
}

const char* var_query_type(const char* ripe_name)
{
  if (not var_query(ripe_name)){
    fatal_throw("unknown variable '%s'", ripe_name);
  }
  return query(ripe_name)->type;
}

const char* var_query_c_name(const char* ripe_name)
{
  if (not var_query(ripe_name)){
    fatal_throw("unknown variable '%s'", ripe_name);
  }
  return query(ripe_name)->c_name;
}
