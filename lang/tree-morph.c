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
    char* text = (char*) mem_strdup(n->text);

    // If no `, then we're good to go.
    if (strchr(text, '`') == NULL) return n;
    
    StringBuf sb;
    sbuf_init(&sb, "");

    const char* s = text;
    Node* expr_list = node_new(EXPR_LIST); // Place to put all the expressions
    while (strchr(s, '`') != NULL){
      char* s1 = strchr(s, '`');
      char* s2 = strchr(s1+1, '`');
      if (s2 == NULL) fatal_throw("unbalanced '`' inside string '%s'", text);
      *s1 = 0;
      *s2 = 0;

      // Any stuff before the first `
      if (s1 > s){
        sbuf_printf(&sb, "%s", s);
      }

      // Replacement for the expression
      sbuf_printf(&sb, "{}");
      
      // Parse the input
      fatal_push("while parsing expression '%s'", s1+1);
      RipeInput input;
      input_from_string(&input, "string", s1+1);
      Node* expr = build_tree(&input, PARSE_EXPR);
      node_add_child(expr_list, expr);
      fatal_pop();

      s = s2 + 1;
    }
    
    // Any stuff after the last `
    sbuf_printf(&sb, "%s", s);
    
    // Node construction
    Node* new_string = node_new(STRING);
    new_string->text = mem_strdup(sb.str);
    Node* field = node_new(EXPR_FIELD);
    node_add_child(field, new_string);
    node_set_string(field, "name", "f");
    Node* expr_call = node_new(EXPR_CALL);
    node_set_node(expr_call, "callee", field);
    node_set_node(expr_call, "args", expr_list);
    return expr_call;
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
