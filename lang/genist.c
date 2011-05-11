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

// Caller must do all the genist_marker stuff
static void add_parent(const char* child_name, const char* parent_name,
                       bool is_mixin)
{
  if (is_mixin){
    fatal_push("while adding mixin '%s' to class '%s'", parent_name, child_name);
  } else {
    fatal_push("while making '%s' a parent of class '%s'", parent_name, child_name);
  }
  
  ClassInfo* ci_parent = stran_get_class(parent_name);
  ClassInfo* ci = stran_get_class(child_name);

  // Now, check if you are allowed to inherit from the parent.
  ClassType type_child = ci->type;
  ClassType type_parent = ci_parent->type;
  
  // Mixins are only allowed to be CLASS_VIRTUAL
  if (is_mixin and type_parent != CLASS_VIRTUAL){
    fatal_throw("you can only mix in a virtual class");
  }
  
  // For now, nobody is allowed to inherit from CDATA.
  if (type_parent == CLASS_CDATA)
    fatal_throw("can't inherit from CDATA class '%s'", parent_name);

  // CDATA can only inherit from a VIRTUAL.
  if (type_child == CLASS_CDATA and type_parent != CLASS_VIRTUAL)
    fatal_throw("CDATA class '%s' can only inherit from a virtual class "
                "('%s' isn't virtual')", child_name, parent_name);

  // Now, if the child is VIRTUAL and parent is FIELD, then child becomes FIELD.
  if (type_child == CLASS_VIRTUAL and type_parent == CLASS_FIELD){
    ci->type = CLASS_FIELD;
  }

  // Copy properties
  if (type_parent == CLASS_FIELD){
    DictIter* iter = dict_iter_new(&(ci_parent->props));
    while (dict_iter_has(iter)){
      const char* prop_name; PropInfo* pi;
      dict_iter_get_ptrs(iter, (void**) &prop_name, (void**) &pi);
      stran_add_class_property(child_name, prop_name);
    }
  }
  
  // Copy methods
  DictIter* iter = dict_iter_new(&(ci_parent->methods));
  while (dict_iter_has(iter)){
    const char* name; FuncInfo* fi;
    dict_iter_get_ptrs(iter, (void**) &name, (void**) &fi);
    stran_add_class_method(child_name, name, fi, METHOD);
  }
  
  iter = dict_iter_new(&(ci_parent->vg_methods));
  while (dict_iter_has(iter)){
    const char* name; FuncInfo* fi;
    dict_iter_get_ptrs(iter, (void**) &name, (void**) &fi);
    stran_add_class_method(child_name, name, fi, VIRTUAL_GET);
  }

  iter = dict_iter_new(&(ci_parent->vs_methods));
  while (dict_iter_has(iter)){
    const char* name; FuncInfo* fi;
    dict_iter_get_ptrs(iter, (void**) &name, (void**) &fi);
    stran_add_class_method(child_name, name, fi, VIRTUAL_SET);
  }
  
  fatal_pop();
}

static void genist_run_r(const char* class_name)
{
  fatal_push("while tracing genealogy of '%s'", class_name);

  ClassInfo* ci = stran_get_class(class_name);
  // Check and update marker.
  if (ci->genist_marker == GENIST_VISITING) {
    fatal_throw("cycle in genealogy");
  }
  if (ci->genist_marker == GENIST_VISITED) {
    fatal_pop(); return;
  }
  ci->genist_marker = GENIST_VISITING;
  
  DictIter* iter = dict_iter_new(&(ci->mixins));
  while (dict_iter_has(iter)){
    char* mixin = dict_iter_get_ptr(iter);
    add_parent(class_name, mixin, true);
  }

  
  
  const char* parent_name = ci->parent;
  if (strequal(parent_name, "")) {
    // No parent, done.
    ci->genist_marker = GENIST_VISITED;
    fatal_pop(); return;
  }
  
  genist_run_r(parent_name);
  add_parent(class_name, parent_name, false);
  
  ci->genist_marker = GENIST_VISITED;
  fatal_pop();
}

void genist_run()
{
  fatal_push("during genealogy tracing step");

  // Iterate through each class, and process its genealogy.
  DictIter* iter = dict_iter_new(stran_get_classes());
  while(dict_iter_has(iter)){
    const char* class_name; ClassInfo* ci;
    dict_iter_get_ptrs(iter, (void**) &class_name, (void**) &ci);
    
    // This would be a bug, all VISITING have to be marked to VISITED.
    assert(ci->genist_marker != GENIST_VISITING);
    
    genist_run_r(class_name);
  }
  fatal_pop();
}
