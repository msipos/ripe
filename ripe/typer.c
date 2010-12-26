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

static Dict table;

void typer_init()
{
  dict_init(&table, sizeof(char*), sizeof(TyperRecord*),
            dict_hash_string, dict_equal_string);
}

void typer_add(TyperRecord* tr)
{
  const char* name = tr->name;
  if (dict_query(&table, &name, &tr)){
    err("duplicate entry in type records: '%s'", name);
  }
  dict_set(&table, &name, &tr);
}

static void typer_add2(const char* name, const char* rv, int num_params,
                       const char** param_types)
{
  TyperRecord* tr = mem_new(TyperRecord);
  tr->name = mem_strdup(name);
  tr->num_params = num_params;
  tr->param_types = param_types;
  if (rv != NULL) tr->rv = mem_strdup(rv);
  else tr->rv = NULL;
  typer_add(tr);
}

TyperRecord* typer_query(const char* name)
{
  TyperRecord* tr;
  if (not dict_query(&table, &name, &tr)){
    err("cannot find entry in type records: '%s'", name);
  }
  return tr;
}

static const char* util_type(const char* type)
{
  if (type == NULL) return "?";
  return type;
}

void typer_dump(FILE* f)
{
  for (int64 i = 0; i < table.alloc_size; i++){
    if (not dict_has_bucket(&table, i)) continue;
    //const char** key = (const char**) dict_get_bucket_key(&table, i);
    TyperRecord* tr = *((TyperRecord**) dict_get_bucket_value(&table, i));

    fprintf(f, "%s %s %d", tr->name, util_type(tr->rv), tr->num_params);
    for (int j = 0; j < tr->num_params; j++){
      fprintf(f, " %s", util_type(tr->param_types[j]));
    }
    fprintf(f, "\n");
  }
}

void typer_load(FILE* f)
{
  #define BUFSIZE 10240
  char line[BUFSIZE];
  const char* delim = " \n";
  while (fgets(line, BUFSIZE, f)){
    char* tok = strtok(line, delim);
    if (tok == NULL) continue;
    const char* name = mem_strdup(tok);

    tok = strtok(NULL, delim);
    if (tok == NULL) err("error while parsing type information for '%s'", name);
    const char* rv = mem_strdup(tok);

    tok = strtok(NULL, delim);
    if (tok == NULL) err("error while parsing type information for '%s'", name);
    int num_params = atoi(tok);

    const char** param_types = mem_malloc(num_params*sizeof(const char*));
    for (int i = 0; i < num_params; i++){
      tok = strtok(NULL, delim);
      if (tok == NULL) err("error while parsing type information for '%s'",
                            name);
      if (strcmp(tok, "?") == 0){
        param_types[i] = NULL;
      } else {
        param_types[i] = mem_strdup(tok);
      }
    }
    typer_add2(name, rv, num_params, param_types);
  }
}

static const char** typer_params(Node* param_list, int* num_params,
                                 const char* first)
{
  *num_params = node_num_children(param_list);
  if (first != NULL) (*num_params)++;
  const char** out = mem_malloc((*num_params)*sizeof(const char*));
  if (first != NULL) out[0] = mem_strdup(first);

  for (int i = 0; i < param_list->children.size; i++){
    int j = i;
    if (first != NULL) j++;
    Node* n = node_get_child(param_list, i);
    assert(n->type == PARAM);
    if (n->children.size == 0){
      if (node_has_string(n, "array")) out[j] = "*";
      else out[j] = NULL;
    } else {
      out[j] = eval_type(node_get_child(n, 0));
    }
  }
  return out;
}

static void typer_add3(const char* name, const char* rv, Node* params,
                       const char* first)
{
  int num_params;
  const char** param_types = typer_params(params, &num_params, first);
  typer_add2(name, rv, num_params, param_types);
}

static void typer_ast_class(const char* class_name, Node* ast)
{
  for (int i = 0; i < ast->children.size; i++){
    Node* n = node_get_child(ast, i);
    switch(n->type){
      case FUNCTION:
        {
          const char* name = node_get_string(n, "name");
          Node* param_list = node_get_child(n, 1);
          const char* rv = eval_type(node_get_child(n, 0));
          if (node_has_string(n, "annotation")){
            const char* annotation = node_get_string(n, "annotation");
            if (strcmp(annotation, "constructor") == 0){
              typer_add3(mem_asprintf("%s.%s", class_name, name),
                         class_name, param_list, NULL);
            } else {
              // Ignore.
            }
          } else {
            typer_add3(mem_asprintf("%s#%s", class_name, name),
                       rv, param_list, class_name);
          }
        }
        break;
      default:
        break;
    }
  }
}

void typer_ast(Node* ast)
{
  for (int i = 0; i < ast->children.size; i++){
    Node* n = node_get_child(ast, i);
    switch(n->type){
      case FUNCTION:
        {
          const char* name = mem_asprintf("%s%s", namespace_get_prefix(),
                                                  node_get_string(n, "name"));
          const char* rv = eval_type(node_get_child(n, 0));
          Node* param_list = node_get_child(n, 1);
          typer_add3(name, rv, param_list, NULL);
        }
        break;
      case NAMESPACE:
        {
          const char* name = node_get_string(n, "name");
          namespace_push(name);
          typer_ast(node_get_child(n, 0));
          namespace_pop(name);
        }
        break;
      case CLASS:
        {
          const char* class_name = mem_asprintf("%s%s", namespace_get_prefix(),
                                                node_get_string(n, "name"));
          typer_ast_class(class_name, node_get_child(n, 0));
        }
      default:
        break;
    }
  }
}

bool typer_needs_check(const char* destination, const char* source)
{
  if (destination == NULL) return false;
  if (source == NULL) return true;
  if (strequal(destination, source)) return false;
  err("require type '%s' but got type '%s'", destination, source);
  assert_never();
}

const char* typer_infer(Node* expr)
{
  switch(expr->type){
    case K_NIL:
      return "Nil";
    case K_EOF:
      return "Eof";
    case ID:
      {
        Variable* var = query_local_full(expr->text);
        if (var == NULL) return NULL;
        return var->type;
      }
    case DOUBLE:
      return "Double";
    case STRING:
      return "String";
    case SYMBOL:
    case INT:
    case CHARACTER:
      return "Integer";
    case EXPR_ARRAY:
      return "Array1";
    case EXPR_RANGE_BOUNDED:
    case EXPR_RANGE_BOUNDED_LEFT:
    case EXPR_RANGE_BOUNDED_RIGHT:
    case EXPR_RANGE_UNBOUNDED:
      return "Range";
    case K_TRUE:
    case K_FALSE:
    case EXPR_IS_TYPE:
      return "Bool";
    case EXPR_FIELD_CALL:
    case EXPR_INDEX:
    case EXPR_ID_CALL:
    case EXPR_FIELD:
    case EXPR_AT_VAR:
    case C_CODE:
    default:
      return NULL;
  }
  assert_never();
}
