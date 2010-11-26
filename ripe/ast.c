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

static bool is_module_init = false;
static Array module_stack;

static void module_init()
{
  if (not is_module_init){
    is_module_init = true;
    array_init(&module_stack, const char*);
  }
}

const char* module_get_prefix()
{
  module_init();
  const char* module_prefix;
  if (module_stack.size > 0){
    module_prefix = array_get(&module_stack, char*, 0);
    for (int i = 1; i < module_stack.size; i++){
      module_prefix = mem_asprintf("%s.%s", module_prefix,
                                   array_get(&module_stack, char*, i));
    }
    module_prefix = mem_asprintf("%s.", module_prefix);
  } else {
    module_prefix = "";
  }
  return module_prefix;
}

void module_push(const char* name)
{
  module_init();
  array_append(&module_stack, mem_strdup(name));
}

void module_pop()
{
  module_init();
  assert(module_stack.size > 0);
  array_pop(&module_stack, const char*);
}

const char* eval_type(Node* type_node)
{
  const char* type = node_get_string(type_node, "name");
  if (node_num_children(type_node) == 0) return type;
  return mem_asprintf("%s.%s", type,
                      node_get_child(type_node, 0));
}
