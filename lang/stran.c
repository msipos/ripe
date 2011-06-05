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

Dict classes;
Dict functions;
Dict globals;
Dict strings;

///////////////////////////////////////////////////////////////////////
// Static helper functions
ClassInfo* class_info;  // initialized by absorb_ast

static const char* stran_string(const char* s)
{
  assert(s != NULL);

  const char* out;
  if (dict_query(&(strings), &s, &out)){
    assert (out != NULL);
    return out;
  }
  s = mem_strdup(s);
  dict_set(&(strings), &s, &s);
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

static ClassInfo* class_info_new(void)
{
  ClassInfo* ci = mem_new(ClassInfo);
  ci->num_props = 0;
  ci->genist_marker = GENIST_UNVISITED;
  dict_init_string(&(ci->methods), sizeof(FuncInfo*));
  dict_init_string(&(ci->vg_methods), sizeof(FuncInfo*));
  dict_init_string(&(ci->vs_methods), sizeof(FuncInfo*));
  dict_init_string(&(ci->props), sizeof(PropInfo*));
  dict_init_string(&(ci->mixins), sizeof(int));
  return ci;
}

///////////////////////////////////////////////////////////////////////
// stran_absorb stuff
///////////////////////////////////////////////////////////////////////

static FuncInfo* func_info_params(Node* n, const char* class_name)
{
  bool method = (class_name != NULL);
  Node* param_list = node_get_node(n, "param_list");

  FuncInfo* fi = mem_new(FuncInfo);
  // TODO: fi->ret
  fi->ret = stran_string("?");

  // Populate parameters
  fi->num_params = node_num_children(param_list);
  if (method) fi->num_params++;
  fi->param_types = mem_malloc(sizeof(char*) * fi->num_params);
  fi->param_names = mem_malloc(sizeof(char*) * fi->num_params);
  if (method) {
    fi->param_types[0] = stran_string(class_name);
    fi->param_names[0] = stran_string("self");
  }
  int end = fi->num_params;
  if (method) end--;
  for (int i = 0; i < end; i++){
    int j = i; // Destination
    if (method) j = i+1;

    Node* param = node_get_child(param_list, i);
    const char* param_type = stran_param(param);
    fi->param_types[j] = param_type;
    fi->param_names[j] = stran_string(node_get_string(param, "name"));
  }

  return fi;
}

// Helper that combines functions and constructors
static void helper_func(Node* n, const char* name, FunctionType type)
{
  FuncInfo* fi = func_info_params(n, NULL);
  fi->ripe_name = name;
  fi->c_name = stran_string(mem_asprintf("ripe_%s", util_escape(name)));
  fi->v_name = stran_string(mem_asprintf("rv_%s", util_escape(name)));
  fi->type = type;
  stran_add_function(name, fi);
}

static void absorb_function(Node* n, const char* name)
{
  helper_func(n, name, FUNCTION);
}

static void absorb_method(Node* n, const char* name, const char* class_name,
                          FunctionType type)
{
  FuncInfo* fi = func_info_params(n, class_name);
  stran_add_class_method(class_name, name, fi, type);

  // Construct static name
  const char* method_name = name;
  if (type == VIRTUAL_GET) method_name = mem_asprintf("get_%s", name);
  else if (type == VIRTUAL_SET) method_name = mem_asprintf("set_%s", name);
  const char* static_name = mem_asprintf("%s.%s",
                                         class_name,
                                         method_name);
  static_name = stran_string(static_name);

  // Add static name to global functions table
  if (dict_query(&(functions), &static_name, NULL))
    fatal_throw("method '%s' already exists as static symbol", name);
  dict_set(&(functions), &static_name, &fi);

  // Populate remaining fields of fi.
  fi->ripe_name = stran_string(static_name);
  fi->c_name = stran_string(mem_asprintf("ripe_%s", util_escape(static_name)));
  fi->v_name = stran_string(mem_asprintf("rv_%s", util_escape(static_name)));
  fi->type = type;
}

static void absorb_constructor(Node* n, const char* name, const char* class_name)
{
  const char* static_name = mem_asprintf("%s.%s", class_name, name);
  helper_func(n, static_name, CONSTRUCTOR);
}

static void absorb_class_enter(Node* n, const char* class_name)
{
  class_name = stran_string(class_name);
  if (dict_query(&(classes), &class_name, NULL)){
    fatal_throw("class '%s' already defined", class_name);
  }
  class_info = class_info_new();
  dict_set(&(classes), &class_name, &class_info);
  class_info->type = CLASS_VIRTUAL;
  class_info->c_name = stran_string(mem_asprintf("klass_%s", util_escape(class_name)));
  class_info->ripe_name = stran_string(class_name);
  class_info->typedef_name = stran_string(mem_asprintf("klass_%s_typedef",
                                               util_escape(class_name)));
  class_info->parent = NULL;
  if (node_has_node(n, "annotation")){
    Node* annot_list = node_get_node(n, "annotation");
    if (not annot_check(annot_list, 2, "=parent", "=mixin")) {
      fatal_throw("invalid annotations in class '%s'", class_name);
    }

    // Get parent (if any)
    class_info->parent = annot_get(annot_list, "parent");

    // Get mixins (if any)
    for(int num = 0;; num++){
      const char* mixin = annot_get_full(annot_list, "mixin", num);
      if (mixin == NULL) break;
      dict_set(&(class_info->mixins), &mixin, &num);
    }
    
  }
  if (class_info->parent == NULL) class_info->parent = "";
}

static void absorb_class_exit(Node* n, const char* class_name)
{
  class_name = NULL;
  class_info = NULL;
}

static void absorb_var(Node* n, const char* var_name)
{
  var_name = stran_string(var_name);
  if (dict_query(&(globals), &var_name, NULL)){
    fatal_throw("global variable '%s' already defined", var_name);
  }

  GlobalInfo* gi = mem_new(GlobalInfo);
  gi->c_name = util_c_name(var_name);
  dict_set(&(globals), &var_name, &gi);
}

static void absorb_property(Node* n, const char* name, const char* class_name)
{
  if (class_info->type == CLASS_CDATA){
    fatal_throw("class '%s' has c-data and fields", class_name);
  }
  class_info->type = CLASS_FIELD;

  stran_add_class_property(class_name, name);
}

static void absorb_ccode(Node* n, const char* class_name)
{
  if (class_name != NULL){
    if (class_info->type == CLASS_FIELD){
      fatal_throw("class '%s' has fields and c-data", class_name);
    }
    class_info->type = CLASS_CDATA;
  }
}

void stran_absorb_ast(Node* ast, const char* filename)
{
  fatal_push("during structure analysis of '%s'", filename);
  class_info = NULL;

  Aster aster;
  aster_init(&aster);
  aster.cb_function = absorb_function;
  aster.cb_method = absorb_method;
  aster.cb_constructor = absorb_constructor;
  aster.cb_class_enter = absorb_class_enter;
  aster.cb_class_exit = absorb_class_exit;
  aster.cb_var = absorb_var;
  aster.cb_property = absorb_property;
  aster.cb_ccode = absorb_ccode;

  aster_process(ast, &aster);
  fatal_pop();
}

///////////////////////////////////////////////////////////////////////
// stran dumping to and loading from disk
///////////////////////////////////////////////////////////////////////

static void dump_methods(FILE* f, Dict* d)
{
  encode_int(f, d->size);
  DictIter* iter = dict_iter_new(d);
  while (dict_iter_has(iter)){
    char* method_name; FuncInfo* fi;
    dict_iter_get_ptrs(iter, (void**) &method_name, (void**) &fi);
    
    encode_string(f, method_name);
    encode_string(f, fi->ripe_name);
  }
}

// Inverse of above
static void absorb_methods(FILE* f, Dict* methods)
{
  int64 num_methods = decode_int(f);
  for (int j = 0; j < num_methods; j++){
    const char* method_name = decode_string(f);
    const char* static_name = decode_string(f);
    FuncInfo* fi = stran_get_function(static_name);
    if (dict_query(methods, &method_name, NULL)){
      fatal_throw("duplicate method '%s'", method_name);
    }
    dict_set(methods, &method_name, &fi);
  }
}

void stran_dump_to_file(FILE* f)
{
  // Encode functions
  encode_int(f, functions.size);
  
  DictIter* iter_functions = dict_iter_new(&functions);
  while (dict_iter_has(iter_functions)){
    char* func_name; FuncInfo* fi;
    dict_iter_get_ptrs(iter_functions, (void**) &func_name, (void**) &fi);
    
    encode_string(f, func_name);
    encode_string(f, fi->c_name);
    encode_string(f, fi->v_name);
    encode_string(f, fi->ret);
    encode_int(f, fi->type);
    encode_int(f, fi->num_params);
    for (int j = 0; j < fi->num_params; j++){
      encode_string(f, fi->param_types[j]);
      encode_string(f, fi->param_names[j]);
    }
  }
  
  // Encode classes
  encode_int(f, classes.size);

  DictIter* iter_classes = dict_iter_new(&classes);
  while (dict_iter_has(iter_classes)){
    char* class_name; ClassInfo* ci;
    dict_iter_get_ptrs(iter_classes, (void**) &class_name, (void**) &ci);
     
    encode_string(f, class_name);
    encode_string(f, ci->parent);
    encode_string(f, ci->c_name);
    encode_int(f, ci->type);
    encode_string(f, ci->typedef_name);

    dump_methods(f, &(ci->methods));
    dump_methods(f, &(ci->vg_methods));
    dump_methods(f, &(ci->vs_methods));

    encode_int(f, ci->props.size);
    DictIter* iter_props = dict_iter_new(&(ci->props));
    while (dict_iter_has(iter_props)){
      char* prop_name; PropInfo* pi;
      dict_iter_get_ptrs(iter_props, (void**) &prop_name, (void**) &pi);
      
      encode_string(f, prop_name);
      encode_int(f, pi->type);
    }
    
    encode_int(f, ci->mixins.size);
    DictIter* iter_mixins = dict_iter_new(&(ci->mixins));
    while (dict_iter_has(iter_mixins)){
      char* mixin = dict_iter_get_ptr(iter_mixins);
      encode_string(f, mixin);
    }
  }
  
  // Encode globals
  encode_int(f, globals.size);
  DictIter* iter_globals = dict_iter_new(&globals);
  while (dict_iter_has(iter_globals)){
    char* global_name; GlobalInfo* gi;
    dict_iter_get_ptrs(iter_globals, (void**) &global_name, (void**) &gi);

    encode_string(f, global_name);
    encode_string(f, gi->c_name);
  }
}

void stran_absorb_file(const char* filename)
{
  fatal_push("while absorbing metadata from '%s'", filename);

  FILE* f = fopen(filename, "r");
  if (f == NULL) fatal_throw("failed to open '%s' for reading: %s'",
                             filename, strerror(errno));
  class_info = NULL;

  // Decode functions
  int64 num_functions = decode_int(f);
  for (int64 i = 0; i < num_functions; i++){
    FuncInfo* fi = mem_new(FuncInfo);
    const char* func_name = decode_string(f);
    
    fi->c_name = decode_string(f);
    fi->v_name = decode_string(f);
    fi->ret = decode_string(f);
    fi->type = decode_int(f);
    fi->num_params = decode_int(f);
    fi->param_types = (const char**) mem_malloc(sizeof(char*) * fi->num_params);
    fi->param_names = (const char**) mem_malloc(sizeof(char*) * fi->num_params);
    for (int j = 0; j < fi->num_params; j++){
      fi->param_types[j] = decode_string(f);
      fi->param_names[j] = decode_string(f);
    }
    stran_add_function(func_name, fi);
  }
  
  // Decode classes
  int64 num_classes = decode_int(f);
  for (int64 i = 0; i < num_classes; i++){
    ClassInfo* ci = class_info_new();
    const char* class_name = decode_string(f);
    ci->parent = decode_string(f);
    ci->ripe_name = class_name;
    ci->c_name = decode_string(f);
    ci->type = decode_int(f);
    ci->typedef_name = decode_string(f);
    dict_set(&(classes), &class_name, &ci);
  
    absorb_methods(f, &(ci->methods));
    absorb_methods(f, &(ci->vg_methods));
    absorb_methods(f, &(ci->vs_methods));
    
    int64 num_props = decode_int(f);
    for (int j = 0; j < num_props; j++){
      const char* prop_name = decode_string(f);
      int type = decode_int(f); // Ignored for now
      stran_add_class_property(class_name, prop_name);
    }
    
    int64 num_mixins = decode_int(f);
    for (int j = 0; j < num_mixins; j++){
      const char* mixin = decode_string(f);
      dict_set(&(ci->mixins), &mixin, &j);
    }
  }
  
  // Decode globals
  int64 num_globals = decode_int(f);
  for (int64 i = 0; i < num_globals; i++){
    GlobalInfo* gi = mem_new(GlobalInfo);
    const char* global_name = decode_string(f);
    gi->c_name = decode_string(f);
    dict_set(&globals, &global_name, &gi);
  }
  
  fatal_pop();
}

///////////////////////////////////////////////////////////////////////
// interface to stran and init
///////////////////////////////////////////////////////////////////////

FuncInfo* stran_get_method(const char* class_name, const char* name)
{
  slog("stran_get_method(class_name='%s', name='%s')", class_name, name);

  ClassInfo* ci = stran_get_class(class_name);
  if (ci == NULL) {
    fatal_throw("no class '%s' (looked for method '%s')", class_name, name);
  }
  FuncInfo* fi = NULL;
  dict_query(&(ci->methods), &name, &fi);
  if (fi == NULL) {
    fatal_throw("no method '%s' in class '%s'", name, class_name);
  }
  return fi;
}

FuncInfo* stran_get_function(const char* name)
{
  FuncInfo* fi = NULL;
  dict_query(&(functions), &name, &fi);
  if (fi == NULL){
    fatal_throw("requested unknown function '%s'", name);
  }
  return fi;
}

GlobalInfo* stran_query_global(const char* name)
{
  GlobalInfo* gi = NULL;
  dict_query(&(globals), &name, &gi);
  return gi;
}

GlobalInfo* stran_get_global(const char* name)
{
  GlobalInfo* gi = stran_query_global(name);
  if (gi == NULL) fatal_throw("requested unknown global variable '%s'", name);
  return gi;
}

ClassInfo* stran_get_class(const char* name)
{
  ClassInfo* ci = NULL;
  dict_query(&(classes), &name, &ci);
  if (ci == NULL){
    fatal_throw("requested unknown class '%s'", name);
  }
  return ci;
}

void stran_add_function(const char* name, FuncInfo* fi)
{
  name = stran_string(name);
  if (dict_query(&(functions), &name, NULL)){
    fatal_throw("function '%s' already defined", name);
  }
  dict_set(&(functions), &name, &fi);
}

void stran_add_class_method(const char* class_name, const char* name, 
                            FuncInfo* fi, FunctionType type)
{
  ClassInfo* ci = stran_get_class(class_name);

  const char* method_name = name;
  if (type == VIRTUAL_GET) method_name = mem_asprintf("get_%s", name);
  else if (type == VIRTUAL_SET) method_name = mem_asprintf("set_%s", name);
  method_name = stran_string(method_name);
  if (dict_query(&(ci->methods), &method_name, NULL))
    fatal_throw("method '%s' already exists in class '%s'", method_name, class_name);
  dict_set(&(ci->methods), &method_name, &fi);

  if (type == VIRTUAL_GET){
    if (dict_query(&(ci->vg_methods), &name, NULL))
      fatal_throw("virtual get method '%s' already exists in class '%s'", name, class_name);
    dict_set(&(class_info->vg_methods), &name, &fi);
  }

  if (type == VIRTUAL_SET){
    if (dict_query(&(ci->vs_methods), &name, NULL))
      fatal_throw("virtual set method '%s' already exists in class '%s'", name, class_name);
    dict_set(&(class_info->vs_methods), &name, &fi);
  }
}

void stran_add_class_property(const char* class_name, const char* name)
{
  ClassInfo* ci = stran_get_class(class_name);
  
  if (dict_query(&(ci->props), &name, NULL)){
    fatal_throw("duplicate property '%s' in class '%s'", name, class_name);
  }

  PropInfo* pi = mem_new(PropInfo);
  pi->type = PROP_FIELD;
  dict_set(&(ci->props), &name, &pi);
  ci->num_props += 1;
}

Dict* stran_get_classes()
{
  return &classes;
}

void stran_init()
{
  dict_init_string(&(classes), sizeof(ClassInfo*));
  dict_init_string(&(functions), sizeof(FuncInfo*));
  dict_init_string(&(globals), sizeof(GlobalInfo*));
  dict_init_string(&(strings), sizeof(char*));
}
