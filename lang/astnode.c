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

Node* node_new(int type)
{
  Node* node = mem_new(Node);
  node->type = type;
  node->filename = NULL;
  node->line = -1;
  array_init( &(node->children), Node*);
  dict_init( &(node->props_strings), sizeof(char*), sizeof(char*),
             dict_hash_string, dict_equal_string );
  return node;
}

Node* node_new_token(int type, const char* text, const char* filename, int line)
{
  Node* node = node_new(type);
  node->text = text;
  node->filename = filename;
  node->line = line;
  return node;
}

Node* node_new_inherit(int type, Node* ancestor)
{
  Node* node = node_new(type);
  node->filename = ancestor->filename;
  node->line = ancestor->line;
  return node;
}

void node_add_child(Node* parent, Node* child)
{
  array_append( &(parent->children), child);
}

void node_extend_children(Node* new_parent, Node* old_parent)
{
  for (int i = 0; i < node_num_children(old_parent); i++){
    Node* child = node_get_child(old_parent, i);
    node_add_child(new_parent, child);
  }
}

void node_prepend_child(Node* parent, Node* child)
{
  array_prepend( &(parent->children), sizeof(Node*), &child);
}

uint node_num_children(Node* parent)
{
  return parent->children.size;
}

Node* node_get_child(Node* parent, uint i)
{
  assert(i < parent->children.size);
  return array_get( &(parent->children), Node*, i);
}

void node_set_string(Node* n, const char* key, const char* value)
{
  assert(n != NULL);
  assert(key != NULL);
  dict_set( &(n->props_strings), &key, &value);
}

char* node_get_string(Node* n, const char* key)
{
  char* out;
  #ifdef NDEBUG
    dict_query(&(n->props_strings), &key, &out);
  #else
    bool rv = dict_query(&(n->props_strings), &key, &out);
    assert(rv == true);
  #endif
  return out;
}

bool node_has_string(Node* n, const char* key)
{
  return dict_query(&(n->props_strings), &key, NULL);
}

static void node_draw_r(Node* ast, int level)
{
  for (int i = 0; i < level; i++){
    printf("  ");
  }
  if (ast->text != NULL){
    printf("Type: %4d '%s'\n", ast->type, ast->text);
  } else {
    printf("Type: %4d\n", ast->type);
  }
  for (int i = 0; i < node_num_children(ast); i++)
  {
    node_draw_r(node_get_child(ast, i), level+1);
  }
}

void node_draw(Node* ast)
{
  node_draw_r(ast, 0);
}

Node* node_new_id(const char* id)
{
  Node* rv = node_new(ID);
  rv->text = mem_strdup(id);
  return rv;
}

Node* node_new_int(int64 i)
{
  Node* rv = node_new(INT);
  rv->text = mem_asprintf("%"PRId64, i);
  return rv;
}

Node* node_new_expr_index1(Node* left, Node* index)
{
  Node* expr_list = node_new(EXPR_LIST);
  node_add_child(expr_list, index);
  Node* rv = node_new(EXPR_INDEX);
  node_add_child(rv, left);
  node_add_child(rv, expr_list);
  return rv;
}

Node* node_new_expr_list()
{
  return node_new(EXPR_LIST);
}

Node* node_new_field_call(Node* callee, char* field_name, int64 num, ...)
{
  Node* rv = node_new(EXPR_FIELD_CALL);
  node_add_child(rv, callee);
  node_set_string(rv, "name", field_name);
  Node* expr_list = node_new(EXPR_LIST);
  va_list ap;
  va_start(ap, num);
  for (int64 i = 0; i < num; i++){
    node_add_child(expr_list, va_arg(ap, Node*));
  }
  va_end(ap);
  node_add_child(rv, expr_list);
  return rv;
}

Node* node_new_type(const char* type)
{
  Node* rv = node_new(TYPE);
  node_set_string(rv, "name", mem_strdup(type));
  return rv;
}
