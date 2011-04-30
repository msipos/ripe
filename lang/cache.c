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

static Dict prototypes;
void cache_prototype(const char* name)
{
  if (dict_query(&prototypes, &name, NULL)) return;
  int i = 1;
  dict_set(&prototypes, &name, &i);

  wr_print(WR_HEADER, "%s;\n", util_signature(name));
}

// Returns the name of the global static C variable of type Value that
// corresponds to that symbol.
static Dict tbl_dsym; // symbol name -> integer 0...
const char* cache_dsym(const char* symbol)
{
  char* dsym_c_var;
  if (dict_query(&tbl_dsym, &symbol, &dsym_c_var))
    return dsym_c_var;

  static uint64 counter = 0;
  counter++;

  dsym_c_var = mem_asprintf("_dsym%"PRIu64"_%s",
                            counter,
                            util_escape(symbol));
  wr_print(WR_HEADER, "static Value %s;\n", dsym_c_var);
  wr_print(WR_INIT2, "  %s = dsym_get(\"%s\");\n",
                         dsym_c_var, symbol);
  dict_set(&tbl_dsym, &symbol, &dsym_c_var);
  return dsym_c_var;
}

// Returns the name of the global static C variable of type Klass* that
// corresponds to that typename.
static Dict tbl_types; // type name -> string name of C variable of type Klass*
const char* cache_type(const char* type)
{
  char* klassp_c_var;
  if (dict_query(&tbl_types, &type, &klassp_c_var))
    return klassp_c_var;

  static uint64 counter = 0;
  counter++;
  klassp_c_var = mem_asprintf("_klass%"PRIu64"_%s",
                              counter,
                              util_escape(type));
  wr_print(WR_HEADER, "static Klass* %s;\n",
              klassp_c_var);
  wr_print(WR_INIT2, "  %s = klass_get(dsym_get(\"%s\"));\n",
              klassp_c_var, type);
  dict_set(&tbl_types, &type, &klassp_c_var);
  return klassp_c_var;
}

static Dict global_prototypes;
void cache_global_prototype(const char* global)
{
  if (dict_query(&global_prototypes, &global, NULL)) return;
  int i = 1;
  dict_set(&global_prototypes, &global, &i);

  GlobalInfo* gi = stran_get_global(global);
  wr_print(WR_HEADER, "extern Value %s;\n", gi->c_name);
}

void cache_init()
{
  dict_init_string(&tbl_dsym, sizeof(char*));
  dict_init_string(&tbl_types, sizeof(char*));
  dict_init_string(&prototypes, sizeof(int));
  dict_init_string(&global_prototypes, sizeof(int));
}
