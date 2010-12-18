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

static bool is_namespace_init = false;
static Array namespace_stack;

static void namespace_init()
{
  if (not is_namespace_init){
    is_namespace_init = true;
    array_init(&namespace_stack, const char*);
  }
}

const char* namespace_get_prefix()
{
  namespace_init();
  const char* namespace_prefix;
  if (namespace_stack.size > 0){
    namespace_prefix = array_get(&namespace_stack, char*, 0);
    for (int i = 1; i < namespace_stack.size; i++){
      namespace_prefix = mem_asprintf("%s.%s", namespace_prefix,
                                   array_get(&namespace_stack, char*, i));
    }
    namespace_prefix = mem_asprintf("%s.", namespace_prefix);
  } else {
    namespace_prefix = "";
  }
  return namespace_prefix;
}

void namespace_push(const char* name)
{
  namespace_init();
  array_append(&namespace_stack, mem_strdup(name));
}

void namespace_pop()
{
  namespace_init();
  assert(namespace_stack.size > 0);
  array_pop(&namespace_stack, const char*);
}

const char* eval_type(Node* type_node)
{
  const char* type = node_get_string(type_node, "name");
  if (node_num_children(type_node) == 0) return type;
  return mem_asprintf("%s.%s", type,
                      eval_type(node_get_child(type_node, 0)));
}
