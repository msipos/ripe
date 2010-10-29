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
