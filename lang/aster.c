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

const char* prefix;
const char* class_name;

static bool bad_params(Node* n)
{
  Node* param_list = node_get_node(n, "param_list");
  for (int i = 0; i < node_num_children(param_list)-1; i++){
    Node* param = node_get_child(param_list, i);
    if (node_has_string(param, "array")){
      return true;
    }
  }
  return false;
}

static void aster_process_r(Node* ast, Aster* aster)
{
  for (unsigned int i = 0; i < ast->children.size; i++){
    Node* n = node_get_child(ast, i);
    switch(n->type){
    case FUNCTION:
      if (class_name == NULL) {
        // Function
        const char* name = mem_asprintf("%s%s", prefix,
                                        node_get_string(n, "name"));

        if (bad_params(n)){
          fatal_throw("array parameter not last in function '%s'",
                      name);
        }
        if (aster->cb_function != NULL){
          aster->cb_function(n, name);
        }
      } else {
        // Within a class
        const char* name = node_get_string(n, "name");

        if (bad_params(n)){
          fatal_throw("array parameter not last in method '%s' "
                      "in class '%s'", name, class_name);
        }

        // Now, figure out if virtual_get/virtual_set, constructor or regular
        // method.
        FunctionType type = METHOD;

        if (node_has_node(n, "annotation")){
          Node* annot_list = node_get_node(n, "annotation");

          // Check that no weird annotations have been defined.
          if (not annot_check(annot_list, 3, "constructor", "virtual_get",
                              "virtual_set")){
            fatal_throw("invalid annotations for method '%s' "
                        "in class '%s'", name, class_name);
          }

          // Check that only one of the above was defined.
          int num = 0;
          if (annot_has(annot_list, "constructor")) { num++;
                                                      type = CONSTRUCTOR; }
          if (annot_has(annot_list, "virtual_get")) { num++;
                                                      type = VIRTUAL_GET; }
          if (annot_has(annot_list, "virtual_set")) { num++;
                                                      type = VIRTUAL_SET; }
          if (num != 1){
            fatal_throw("invalid annotations for method '%s' "
                        "in class '%s'", name, class_name);
          }
        }
        
        if (type == CONSTRUCTOR){
          if (aster->cb_constructor != NULL){
            aster->cb_constructor(n, name, class_name);
          }
        } else {
          if (aster->cb_method != NULL){
            aster->cb_method(n, name, class_name, type);
          }
        }
      }
      break;
    case NAMESPACE:
      {
        const char* name = node_get_string(n, "name");
        const char* prev_prefix = prefix;
        prefix = mem_asprintf("%s%s.", prefix, name);

        aster_process_r(node_get_child(n, 0), aster); // Recurse into namespace

        prefix = prev_prefix;
      }
      break;
    case CLASS:
      {
        class_name = mem_asprintf("%s%s", prefix, node_get_string(n, "name"));
        if (aster->cb_class_enter != NULL){
          aster->cb_class_enter(n, class_name);
        }

        aster_process_r(node_get_child(n, 0), aster); // Recurse into class

        if (aster->cb_class_exit != NULL){
          aster->cb_class_exit(n, class_name);
        }
        class_name = NULL;
      }
      break;
    case TOP_VAR:
      if (class_name == NULL) {
        // Global variable
        const char* var_name = mem_asprintf("%s%s", prefix, 
                                      node_get_string(n, "name"));
        if (aster->cb_var != NULL){
          aster->cb_var(n, var_name);
        }
      } else {
        // Class property
        if (aster->cb_property != NULL){
          aster->cb_property(n, node_get_string(n, "name"), class_name);
        }
      }
      break;
    case C_CODE:  
      if (aster->cb_ccode != NULL) {
        aster->cb_ccode(n, class_name);
      }
      break;
    default:
      break;
    }
  }
}

void aster_init(Aster* aster)
{
  aster->cb_function = NULL;
  aster->cb_method = NULL;
  aster->cb_constructor = NULL;
  aster->cb_class_enter = NULL;
  aster->cb_class_exit = NULL;
  aster->cb_var = NULL;
  aster->cb_property = NULL;
  aster->cb_ccode = NULL;
}

void aster_process(Node* ast, Aster* aster)
{
  prefix = "";
  class_name = NULL;
  aster_process_r(ast, aster);
}


