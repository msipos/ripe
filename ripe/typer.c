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

static const char** typer_params(Node* param_list, int* num_params)
{
  *num_params = node_num_children(param_list);
  const char** out = mem_malloc((*num_params)*sizeof(const char*));
  for (int i = 0; i < param_list->children.size; i++){
    Node* n = node_get_child(param_list, i);
    assert(n->type == PARAM);
    if (n->children.size == 0){
      if (node_has_string(n, "array")) out[i] = "*";
      else out[i] = NULL;
    } else {
      out[i] = eval_type(node_get_child(n, 0));
    }
  }
  return out;
}

static void typer_add3(const char* name, const char* rv, Node* params)
{
  int num_params;
  const char** param_types = typer_params(params, &num_params);
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
          if (node_has_string(n, "annotation")){
            const char* annotation = node_get_string(n, "annotation");
            if (strcmp(annotation, "constructor") == 0){
              typer_add3(mem_asprintf("%s.%s", class_name, name),
                         class_name,
                         node_get_child(n, 0));
            } else {
              // Ignore.
            }
          } else {
            typer_add3(mem_asprintf("%s#%s", class_name, name),
                       NULL,
                       node_get_child(n, 0));
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
          const char* name = mem_asprintf("%s%s", module_get_prefix(),
                                                  node_get_string(n, "name"));

          typer_add3(name, NULL, node_get_child(n, 0));
        }
        break;
      case MODULE:
        {
          const char* name = node_get_string(n, "name");
          module_push(name);
          typer_ast(node_get_child(n, 0));
          module_pop(name);
        }
        break;
      case CLASS:
        {
          const char* class_name = mem_asprintf("%s%s", module_get_prefix(),
                                                node_get_string(n, "name"));
          typer_ast_class(class_name, node_get_child(n, 0));
        }
      default:
        break;
    }
  }
}
