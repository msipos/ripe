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

static Node* morph(Node* n)
{
  if (n->type == STRING){
    //printf("%s\n", (n->text));
  }
  return n;
}

static void tree_morph_r(Node* ast)
{
  // Deal with children
  for (int i = 0; i < node_num_children(ast); i++){
    Node* n = node_get_child(ast, i);
    Node* m = morph(n);
    if (n != m) array_set2(&(ast->children), &m, i);
    tree_morph_r(m);
  }
  
  // Deal with nodes
  Dict* props_nodes = &(ast->props_nodes);
  DictIter* iter = dict_iter_new(props_nodes);
  while (dict_iter_has(iter)){
    const char* key; Node* n;
    dict_iter_get_ptrs(iter, (void**) &key, (void**) &n);
    Node* m = morph(n);
    if (n != m) dict_set(props_nodes, &key, &m);
    tree_morph_r(m);    
  }
}

void tree_morph(Node* ast)
{
  fatal_push("during tree morphing step");
  tree_morph_r(ast);
  fatal_pop();
}
