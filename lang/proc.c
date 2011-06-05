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

static StringBuf sb_class_cdata;

static void make_func_value(const char* func_name)
{
  slog("make_func_value('%s')", func_name);
  FuncInfo* fi = stran_get_function(func_name);
  assert(fi != NULL);

  wr_print(WR_INIT1B, "  Value %s = func%d_to_val(%s);\n",
           fi->v_name, fi->num_params, fi->c_name);
  if (fi->num_params > 0 and strequal(fi->param_types[fi->num_params-1], "*")){
     wr_print(WR_INIT1B, "  func_set_vararg(%s);\n", fi->v_name);
  }
}

static void proc_function(Node* n, const char* name)
{
  make_func_value(name);
}

static void proc_constructor(Node* n, const char* name, const char* class_name)
{
  make_func_value(mem_asprintf("%s.%s", class_name, name));
}

static void proc_method(Node* n, const char* name, const char* class_name,
                        FunctionType type)
{
  const char* method_name = name;
  if (type == VIRTUAL_GET) method_name = mem_asprintf("get_%s", name);
  else if (type == VIRTUAL_SET) method_name = mem_asprintf("set_%s", name);

  slog("proc_method(class_name = '%s', method_name = '%s')", class_name,
                                                             method_name);

  FuncInfo* fi = stran_get_method(class_name, method_name); assert(fi != NULL);
  ClassInfo* ci = stran_get_class(class_name); assert(ci != NULL);

  const char* static_name = mem_asprintf("%s.%s", class_name, method_name);
  slog("static_name='%s'", static_name);

  make_func_value(static_name);
}

static void proc_class_enter(Node* n, const char* class_name)
{
  ClassInfo* ci = stran_get_class(class_name);
  sbuf_init(&sb_class_cdata, "");
  wr_print(WR_HEADER, "Klass* %s;\n", ci->c_name);
}

static void proc_class_exit(Node* n, const char* class_name)
{
  // Construct the Klass object
  ClassInfo* ci = stran_get_class(class_name);
  const char* sz = "0";
  if (ci->type == CLASS_CDATA) {
    wr_print(WR_HEADER, "typedef struct {\n");
    wr_print(WR_HEADER, sb_class_cdata.str);
    wr_print(WR_HEADER, "} %s;\n", ci->typedef_name);
    sz = mem_asprintf("sizeof(%s)", ci->typedef_name);
  }
  wr_print(WR_INIT1A, "  %s = klass_new(dsym_get(\"%s\"), %s);\n",
           ci->c_name, class_name, sz);

  // Populate all the fields
  if (ci->type == CLASS_FIELD) {
    for (uint i = 0; i < ci->props.alloc_size; i++){
      if (dict_has_bucket(&(ci->props), i)){
        char* prop_name = *(char**) dict_get_bucket_key(&(ci->props), i);
        PropInfo* pi = *(PropInfo**) dict_get_bucket_value(&(ci->props), i);
        wr_print(WR_INIT1B, "  klass_new_field(%s, dsym_get(\"%s\"), %s);\n",
                 ci->c_name, prop_name, "FIELD_READABLE | FIELD_WRITABLE");
      }
    }
  }
  
  DictIter* iter;
  
  // Populate all the methods
  iter = dict_iter_new(&(ci->methods));
  while (dict_iter_has(iter)){
    const char* method_name; FuncInfo* fi;
    dict_iter_get_ptrs(iter, (void**) &method_name, (void**) &fi);
    wr_print(WR_INIT1B, "  klass_new_method(%s, dsym_get(\"%s\"), %s);\n",
           ci->c_name, method_name, fi->v_name);
  }

  iter = dict_iter_new(&(ci->vg_methods));
  while (dict_iter_has(iter)){
    const char* name; FuncInfo* fi;
    dict_iter_get_ptrs(iter, (void**) &name, (void**) &fi);
    wr_print(WR_INIT1B, "  klass_new_virtual_reader(%s, dsym_get(\"%s\"), %s);\n",
             ci->c_name, name, fi->v_name);
  }

  iter = dict_iter_new(&(ci->vs_methods));
  while (dict_iter_has(iter)){
    const char* name; FuncInfo* fi;
    dict_iter_get_ptrs(iter, (void**) &name, (void**) &fi);
    wr_print(WR_INIT1B, "  klass_new_virtual_writer(%s, dsym_get(\"%s\"), %s);\n",
             ci->c_name, name, fi->v_name);
  }
  
  // Set parent if any
  if (not strequal(ci->parent, "")){
    // This MUST be in INIT2. That's the earliest where Klass* gets cached.'
    // INIT3 is when globals get assigned. Exceptions may be thrown there, so
    // at that time parents must be associated.
    wr_print(WR_INIT2, "  %s->parent = %s;\n",
             cache_type(class_name),
             cache_type(ci->parent));
  }
}

static void proc_var(Node* n, const char* name)
{
  GlobalInfo* gi = stran_get_global(name);
  assert(gi != NULL);

  wr_print(WR_HEADER, "Value %s = VALUE_NIL;\n", gi->c_name);
}

static void proc_ccode(Node* n, const char* class_name)
{
  if (class_name != NULL){
    sbuf_printf(&sb_class_cdata, util_trim_ends(n->text));
  } else {
    wr_print(WR_HEADER, "%s\n", util_trim_ends(n->text));
  }
}

void proc_process_ast(Node* ast, const char* filename)
{
  fatal_push("during processing of '%s'", filename);

  Aster aster;
  aster_init(&aster);
  aster.cb_function = proc_function;
  aster.cb_method = proc_method;
  aster.cb_constructor = proc_constructor;
  aster.cb_class_enter = proc_class_enter;
  aster.cb_class_exit = proc_class_exit;
  aster.cb_var = proc_var;
  // aster.cb_property = proc_property; // Not necessary
  aster.cb_ccode = proc_ccode;
  aster_process(ast, &aster);
  
  fatal_pop();
}
