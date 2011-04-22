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

Error* stran_error;
static Stran stran;

///////////////////////////////////////////////////////////////////////
// Static helper functions (initialize g_stran before you use them!)

const char* class_name; // initialized by absorb_ast
ClassInfo* class_info;  // initialized by absorb_ast
static const char* stran_string(const char* s)
{
  assert(s != NULL);

  const char* out;
  if (dict_query(&(stran.strings), &s, &out)){
    assert (out != NULL);
    return out;
  }
  s = mem_strdup(s);
  dict_set(&(stran.strings), &s, &s);
  return s;
}

static const char* stran_param(Node* param)
{
  if (node_has_string(param, "array")){
    return stran_string("*");
  } else if (node_has_node(param, "type")){
    const char* type = util_dot_id(node_get_node(param, "type"));
    return stran_string(type);
  } else {
    return stran_string("?");
  }
}

static FuncInfo* stran_func(Node* param_list, bool method, const char* func_name)
{
  FuncInfo* fi = mem_new(FuncInfo);
  // TODO: fi->ret
  fi->ret = stran_string("?");
  fi->c_name = stran_string(mem_asprintf("ripe_%s", util_escape(func_name)));

  // Populate parameters
  fi->num_params = node_num_children(param_list);
  if (method) fi->num_params++;
  fi->params = mem_malloc(sizeof(char*) * fi->num_params);
  if (method) fi->params[0] = class_name;
  int end = fi->num_params;
  if (method) end--;
  for (int i = 0; i < end; i++){
    Node* param = node_get_child(param_list, i);
    const char* param_type = stran_param(param);
    if (method) fi->params[i+1] = param_type;
    else fi->params[i] = param_type;
  }

  // Check that there are no "*" except at the end
  for (int i = 0; i < fi->num_params - 1; i++){
    if (strequal(fi->params[i], "*")){
      if (method) {
        err_throw(stran_error, "method '%s' (member of class '%s'): array parameter %d not at end",
                  func_name, class_name, i+1);
      } else {
        err_throw(stran_error, "function '%s': array parameter %d not at end",
                  func_name, i+1);
      }
    }
  }

  return fi;
}

void stran_method(Node* n, const char* name)
{
  if (dict_query(&(class_info->methods), &name, NULL))
    err_throw(stran_error, "method '%s' already exists in class '%s'",
              name, class_name);

  const char* static_name = mem_asprintf("%s.%s",
                                         class_name,
                                         name);
  static_name = stran_string(static_name);
  if (dict_query(&(stran.functions), &static_name, NULL))
    err_throw(stran_error, "method '%s' already exists as static symbol", name);

  FuncInfo* fi = stran_func(node_get_child(n, 0), true, static_name);
  dict_set(&(class_info->methods), &name, &fi);
  dict_set(&(stran.functions), &static_name, &fi);
}

const char* prefix;
static void stran_absorb_ast_r(Node* ast)
{
  for (int i = 0; i < ast->children.size; i++){
    Node* n = node_get_child(ast, i);
    switch(n->type){
      case FUNCTION:
        if (class_name == NULL) {
          // Function
          const char* name = mem_asprintf("%s%s", prefix,
                                          node_get_string(n, "name"));
          name = stran_string(name);
          if (dict_query(&(stran.functions), &name, NULL)){
            err_throw(stran_error, "function '%s' already defined", name);
          }
          FuncInfo* fi = stran_func(node_get_child(n, 0), false, name);
          dict_set(&(stran.functions), &name, &fi);
        } else {
          // Within a class
          const char* name = node_get_string(n, "name");
          name = stran_string(name);

          // Now, figure out if virtual_get/virtual_set, constructor or regular
          // method.
          #define METHOD 1
          #define VIRTUAL_GET 2
          #define VIRTUAL_SET 3
          #define CONSTRUCTOR 4
          int this_is = METHOD;

          if (node_has_node(n, "annotation")){
            Node* annot_list = node_get_node(n, "annotation");

            // Check that no weird annotations have been defined.
            annot_check(annot_list, 3,
                        "constructor", "virtual_get", "virtual_set");

            // Check that only one of the above was defined.
            int num = 0;
            if (annot_has(annot_list, "constructor")) { num++;
                                                        this_is = CONSTRUCTOR; }
            if (annot_has(annot_list, "virtual_get")) { num++;
                                                        this_is = VIRTUAL_GET; }
            if (annot_has(annot_list, "virtual_set")) { num++;
                                                        this_is = VIRTUAL_SET; }
            if (num != 1){
              err_throw(stran_error, "invalid annotations for function '%s'",
                        name);
            }
          }

          switch(this_is){
            case METHOD:
              stran_method(n, name);
              break;
            case VIRTUAL_GET:
              stran_method(n, stran_string(mem_asprintf("get_%s", name)));
              break;
            case VIRTUAL_SET:
              stran_method(n, stran_string(mem_asprintf("set_%s", name)));
              break;
            case CONSTRUCTOR:
              {
                const char* static_name = mem_asprintf("%s.%s",
                                                       class_name,
                                                       name);
                static_name = stran_string(static_name);
                if (dict_query(&(stran.functions), &static_name, NULL)){
                  err_throw(stran_error, "constructor '%s' already defined", static_name);
                }

                FuncInfo* fi = stran_func(node_get_child(n, 0), false, static_name);
                dict_set(&(stran.functions), &static_name, &fi);
              }
              break;
            default:
              assert_never();
          }
        }
        break;
      case NAMESPACE:
        {
          const char* name = node_get_string(n, "name");
          const char* prev_prefix = prefix;
          prefix = mem_asprintf("%s%s.", prefix, name);

          stran_absorb_ast_r(node_get_child(n, 0)); // Recurse into namespace

          prefix = prev_prefix;
        }
        break;
      case CLASS:
        {
          class_name = mem_asprintf("%s%s", prefix, node_get_string(n, "name"));
          class_name = stran_string(class_name);
          class_info = mem_new(ClassInfo);
          dict_init_string(&(class_info->methods), sizeof(FuncInfo));

          stran_absorb_ast_r(node_get_child(n, 0)); // Recurse into class

          dict_set(&(stran.classes), &class_name, &class_info);
          class_name = NULL;
          class_info = NULL;
        }
      default:
        break;
    }
  }
}

void stran_dump_to_file(FILE* f)
{
  for (int i = 0; i < stran.functions.alloc_size; i++){
    if (dict_has_bucket(&(stran.functions), i)){
      char* func_name = *((char**) dict_get_bucket_key(&(stran.functions), i));
      FuncInfo* fi = *((FuncInfo**) dict_get_bucket_value(&(stran.functions), i));
      fprintf(f, "function %s %s %s %d", func_name, fi->c_name, fi->ret, fi->num_params);
      for (int j = 0; j < fi->num_params; j++){
        fprintf(f, " %s", fi->params[j]);
      }
      fprintf(f, "\n");
    }
  }
}

int stran_absorb_file(const char* filename)
{
  if (err_check(stran_error)) return 1;
  FILE* f = fopen(filename, "r");
  if (f == NULL) err_throw(stran_error, "failed to open '%s' for reading: %s'",
                           filename, strerror(errno));
  class_name = NULL;
  class_info = NULL;

  char line[1024];
  while (fgets(line, 1024, f)){
    Tok tok;
    tok_init_white(&tok, line);
    // Ignore empty lines
    if (tok.num == 0) continue;
    if (strequal(tok.words[0], "function")){
      // Function record:
      // function func_name c_name return_type num_params param1_type param2_type...
      if (tok.num < 5) {
        err_throw(stran_error, "invalid type data: %s", line);
      }
      FuncInfo* fi = mem_new(FuncInfo);
      const char* func_name = stran_string(tok.words[1]);
      fi->c_name = stran_string(tok.words[2]);
      fi->ret = stran_string(tok.words[3]);
      fi->num_params = atoi(tok.words[4]);

      if (tok.num != 5 + fi->num_params){
        err_throw(stran_error, "invalid type data: %s", line);
      }
      fi->params = (const char**) mem_malloc(sizeof(char*) * fi->num_params);
      for (int i = 0; i < fi->num_params; i++){
        fi->params[i] = stran_string(tok.words[5+i]);
      }
      dict_set(&(stran.functions), &func_name, &fi);
    } else {
      err_throw(stran_error, "invalid type data: %s", line);
    }
  }

  return 0;
}

int stran_absorb_ast(Node* ast)
{
  if (err_check(stran_error)) return 1;

  prefix = "";
  class_name = NULL;
  class_info = NULL;
  stran_absorb_ast_r(ast);
  return 0;
}

FuncInfo* stran_get_function(const char* name)
{
  FuncInfo* fi = NULL;
  dict_query(&(stran.functions), &name, &fi);
  return fi;
}

void stran_prototype(const char* name)
{
  if (dict_query(&(stran.prototypes), &name, NULL)) return;
  int i = 1;
  dict_set(&(stran.prototypes), &name, &i);

  FuncInfo* fi = stran_get_function(name);
  assert(fi != NULL);

  wr_print(WR_HEADER, "Value %s(", fi->c_name);
  for (int i = 0; i < fi->num_params; i++){
    wr_print(WR_HEADER, "Value");
    if (i != fi->num_params - 1){
      wr_print(WR_HEADER, ", ");
    }
  }
  wr_print(WR_HEADER, ");\n");
}

void stran_dump()
{
  printf("FUNCTIONS:\n");
  for (int i = 0; i < stran.functions.alloc_size; i++){
    if (dict_has_bucket(&(stran.functions), i)){
      const char* name = *((const char**) dict_get_bucket_key(&(stran.functions), i));
      printf("  %s\n", name);
    }
  }
}

void stran_init()
{
  dict_init_string(&(stran.classes), sizeof(ClassInfo*));
  dict_init_string(&(stran.functions), sizeof(FuncInfo*));
  dict_init_string(&(stran.strings), sizeof(char*));
  dict_init_string(&(stran.prototypes), sizeof(int));
}
