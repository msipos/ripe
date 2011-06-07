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

EE* ee_new(const char* type, const char* text)
{
  EE* ee = mem_new(EE);
  if (type != UNTYPED) ee->type = mem_strdup(type);
  ee->text = mem_strdup(text);
  return ee;
}

const char* ee_Value(EE* ee)
{
  if (strequal(ee->type, "int64")){
    return mem_asprintf("int64_to_val(%s)", ee->text);
  }
  if (strequal(ee->type, "double")){
    return mem_asprintf("double_to_val(%s)", ee->text);
  }
  if (strequal(ee->type, "bool")){
    return mem_asprintf("pack_bool(%s)", ee->text);
  }
  return ee->text;  
}

const char* ee_type(const char* type, EE* ee)
{
  return NULL;
}

static const char* eval_expr_list(Node* expr_list, bool first_comma)
{
  assert(expr_list->type == EXPR_LIST);
  StringBuf sb_temp;
  sbuf_init(&sb_temp, "");
  for (uint i = 0; i < expr_list->children.size; i++){
    if (i == 0 and not first_comma){
      sbuf_printf(&sb_temp, 
                  "%s", 
                  eval_Value(node_get_child(expr_list, i)));
    } else {
      sbuf_printf(&sb_temp,
                  ", %s",
                  eval_Value(node_get_child(expr_list, i)));
    }
  }
  const char* result = mem_strdup(sb_temp.str);
  sbuf_deinit(&sb_temp);
  return result;
}


static const char* eval_static_call(const char* ssym, Node* arg_list)
{
  FuncInfo* fi = stran_get_function(ssym);
  if (fi == NULL){
    fatal_node(arg_list, "unknown static call to '%s'", ssym);
  }

  int num_args = node_num_children(arg_list);
  int min_params = num_args;
  int num_params = fi->num_params;
  bool is_vararg = false;

  // Check if vararg
  if (num_params > 0 and strequal("*", fi->param_types[num_params-1])){
    is_vararg = true;
    min_params = num_params - 1;
    if (num_args < min_params)
      fatal_node(arg_list, "'%s' called with %d arguments but expect at least %d",
               ssym, num_args, min_params);
  } else {
    if (num_params != num_args)
      fatal_node(arg_list, "'%s' called with %d arguments but expect %d", ssym,
               num_args, num_params);
  }

  cache_prototype(ssym); // Make certain the C compiler knows fi->c_name
  const char* buf = mem_asprintf("%s(", fi->c_name);
  for (int i = 0; i < min_params; i++){
    Node* arg = node_get_child(arg_list, i);
    if (i == 0)
      buf = mem_asprintf("%s%s", buf, eval_Value(arg));
    else
      buf = mem_asprintf("%s, %s", buf, eval_Value(arg));
  }
  if (is_vararg){
    if (min_params == 0)
      buf = mem_asprintf("%stuple_to_val(%d", buf, num_args - min_params);
    else
      buf = mem_asprintf("%s, tuple_to_val(%d", buf, num_args - min_params);

    for (int i = min_params; i < num_args; i++){
      Node* arg = node_get_child(arg_list, i);
      buf = mem_asprintf("%s, %s", buf, eval_Value(arg));
    }
    buf = mem_asprintf("%s)", buf);
  }
  buf = mem_asprintf("%s)", buf);

  return buf;
}

static const char* eval_obj_call(Node* obj, const char* method_name,
                             Node* expr_list)
{
  return mem_asprintf("method_call%d(%s, %s %s)",
                      node_num_children(expr_list),
                      eval_Value(obj),
                      cache_dsym(method_name),
                      eval_expr_list(expr_list, true));
}

// Returns code for accessing index (if assign = NULL), or setting index
// when assign is of type expr.
const char* eval_index(Node* self, Node* idx, Node* assign)
{
  if (assign == NULL) {
    return eval_obj_call(self, "index", idx);
  } else {
    Node* arg_list = node_new(EXPR_LIST);
    for (int i = 0; i < node_num_children(idx); i++){
      node_add_child(arg_list, node_get_child(idx, i));
    }
    node_add_child(arg_list, assign);
    return eval_obj_call(self, "index_set", arg_list);
  }
}

// Attempt to evaluate the expression as an identifier with dots, otherwise
// return NULL.
// I.e. Std.println would evaluate and return "Std.println", but (1+1).to_s
// would not.
static const char* eval_expr_as_id(Node* expr)
{
  switch(expr->type){
    case ID:
      if (var_query(expr->text)) return NULL;
      return expr->text;
    case EXPR_FIELD:
      {
        Node* parent = node_get_child(expr, 0);
        const char* field = node_get_string(expr, "name");
        const char* s = eval_expr_as_id(parent);
        if (s != NULL) return mem_asprintf("%s.%s", s, field);
        else return NULL;
      }
    default:
      // Anything other than ID or EXPR_FIELD means that it cannot be a
      // static symbol.
      return NULL;
  }
}

const char* eval_type(Node* n)
{
  const char* type = eval_expr_as_id(n);
  if (type == NULL) fatal_node(n, "invalid type (node of type %d)", n->type);
  return type;
}

EE* eval_expr(Node* expr)
{
  if (is_unary_op(expr))
    return ee_new(UNTYPED, 
                  mem_asprintf("%s(%s)",
                               unary_op_map(expr->type),
                               eval_Value(node_get_child(expr, 0))));
  if (is_binary_op(expr))
    return ee_new (UNTYPED, 
                   mem_asprintf("%s(%s, %s)",
                                binary_op_map(expr->type),
                                eval_Value(node_get_child(expr, 0)),
                                eval_Value(node_get_child(expr, 1))));

  switch(expr->type){
  case K_TRUE:
    return ee_new("Bool", "VALUE_TRUE");
  case K_FALSE:
    return ee_new("Bool", "VALUE_FALSE");
  case K_NIL:
    return ee_new("Nil", "VALUE_NIL");
  case K_EOF:
    return ee_new("Eof", "VALUE_EOF");
  case ID:
    if (context_block != NULL){
      // If it's a block param, then OK.
      if (var_query_kind(expr->text) == VAR_BLOCK_PARAM)
        return ee_new(var_query_type(expr->text), var_query_c_name(expr->text));
      else 
        return ee_new(var_query_type(expr->text),
                      closure_add(expr->text, var_query_c_name(expr->text)));
    } else return ee_new(var_query_type(expr->text), var_query_c_name(expr->text));
  case SYMBOL:
    return ee_new("Integer", cache_dsym(expr->text + 1));
  case INT:
    return ee_new("Integer", mem_asprintf("int64_to_val(%s)", expr->text));
  case DOUBLE:
    return ee_new("Double", mem_asprintf("double_to_val(%s)", expr->text));
  case STRING:
    {
      const char* str = expr->text;
      return ee_new("String", mem_asprintf("string_to_val(\"%s\")", str));
    }
    break;
  case CHARACTER:
    {
      const char* str = expr->text;
      return ee_new("Integer", mem_asprintf("int64_to_val(%d)", (int) str[1]));
    }
    break;
  case EXPR_ARRAY:
    {
      Node* expr_list = node_get_child(expr, 0);
      return ee_new("Array", mem_asprintf("array1_to_val2(%u %s)",
                                          expr_list->children.size,
                                          eval_expr_list(expr_list, true)));
    }
  case EXPR_MAP:
    {
      Node* m_list = node_get_child(expr, 0);
      bool is_map = false, is_set = false;
      const char* args = "";

      const int num_ms = node_num_children(m_list);
      assert(num_ms > 0);
      for (int i = 0; i < num_ms; i++){
        Node* m = node_get_child(m_list, i);
        switch (node_num_children(m)){
          case 2:
            is_map = true;
            args = mem_asprintf("%s, %s, %s", args,
                                eval_Value(node_get_child(m, 0)),
                                eval_Value(node_get_child(m, 1)));
            break;
          case 1:
            is_set = true;
            args = mem_asprintf("%s, %s", args,
                                eval_Value(node_get_child(m, 0)));
            break;
          default:
            assert_never();
            break;
        }
      }

      if (is_map and is_set){
        fatal_node(expr, "invalid curly braces: is this a set or a map?");
      }
      if (is_map) return ee_new("Map",
                                mem_asprintf("ht_new_map(%d%s)", num_ms, args));
      if (is_set) return ee_new("Set", 
                                mem_asprintf("ht_new_set(%d%s)", num_ms, args));
      assert_never();
    }
    break;
  case EXPR_INDEX:
    return ee_new(UNTYPED, 
                  eval_index(node_get_child(expr, 0), node_get_child(expr, 1), NULL));
  case EXPR_CALL:
    {
      Node* callee = node_get_node(expr, "callee");
      Node* args = node_get_node(expr, "args");
      
      // If callee is a field, then we must check if its a method call.
      if (callee->type == EXPR_FIELD){
        const char* field_name = node_get_string(callee, "name");
        Node* left = node_get_child(callee, 0);
        
        const char* s = eval_expr_as_id(left);
        if (s == NULL or var_query(s))
          return ee_new(UNTYPED, eval_obj_call(left, field_name, args));
      }
      
      const char* s = eval_expr_as_id(callee);
      if (s != NULL){
        // Could be a tuple constructor.
        if (strequal(s, "tuple")){
          return ee_new("Tuple", mem_asprintf("tuple_to_val(%u %s)",
                                              node_num_children(args),
                                              eval_expr_list(args, true)));
        }

        // If all of callee can be evaluated as an id, then it must be a static
        // call.
        return ee_new(UNTYPED, eval_static_call(s, args));
      }
      

      return ee_new(UNTYPED, mem_asprintf("func_call%d(%s %s)", 
                                          node_num_children(args),
                                          eval_Value(callee),
                                          eval_expr_list(args, true)));
    }
  case EXPR_RANGE_BOUNDED:
    {
      Node* left = node_get_child(expr, 0);
      Node* right = node_get_child(expr, 1);
      return ee_new("Range",
                    mem_asprintf("range_to_val(RANGE_BOUNDED, "
                                 "val_to_int64(%s), val_to_int64(%s))",
                                 eval_Value(left),
                                 eval_Value(right)));
    }
  case EXPR_RANGE_BOUNDED_LEFT:
    return ee_new("Range", 
                  mem_asprintf("range_to_val(RANGE_BOUNDED_LEFT, "
                               "val_to_int64(%s), 0)",
                               eval_Value(node_get_child(expr, 0))));
  case EXPR_RANGE_BOUNDED_RIGHT:
    return ee_new("Range",
                  mem_asprintf("range_to_val(RANGE_BOUNDED_RIGHT, "
                               "0, val_to_int64(%s))",
                               eval_Value(node_get_child(expr, 0))));
  case EXPR_RANGE_UNBOUNDED:
    return ee_new("Range", "range_to_val(RANGE_UNBOUNDED, 0, 0)");
  case EXPR_FIELD:
    {
      // Attempt to evaluate this field as a static symbol.
      Node* left = node_get_child(expr, 0);
      const char* s = eval_expr_as_id(left);
      if (s == NULL or var_query(s)){
        // Dynamic field.
        const char* field = node_get_string(expr, "name");

        return ee_new(UNTYPED, 
                      mem_asprintf("field_get(%s, %s)",
                                   eval_Value(left),
                                   cache_dsym(field)));
      } else {
        // Could be a global variable.
        s = eval_expr_as_id(expr);

        if (context_block != NULL){
          return ee_new(var_query_type(s), closure_add(s, var_query_c_name(s)));
        } else return ee_new(var_query_type(s), var_query_c_name(s));
      }
    }
    break;
  case EXPR_AT_VAR:
    {
      const char* name = node_get_string(expr, "name");
      if (context_ci == NULL){
        fatal_node(expr, "'@%s' in something that's not a class", name);
      }
      if (context_ci->type != CLASS_FIELD){
        fatal_node(expr, "'@%s' in a class that's not a field class", name);
      }
      return ee_new(UNTYPED,
                    mem_asprintf("field_get(__self, %s)", cache_dsym(name)));
    }
  case C_CODE:
    {
      const char* str = util_trim_ends(expr->text);
      if (strstr(expr->text, "return")){
        fatal_warn("careless return in C code may disrupt the stack (use RRETURN)");
      }

      if (context_fi != NULL){
        if (context_fi->type == METHOD
             or context_fi->type == CONSTRUCTOR
             or context_fi->type == VIRTUAL_GET
             or context_fi->type == VIRTUAL_SET){
          if (strchr(str, '@') != NULL and context_ci->type != CLASS_CDATA)
            fatal_node(expr, "@ in C code in a class that is not cdata");
          str = util_replace(str, '@', "_c_data->");
        }
      }

      return ee_new(UNTYPED, str);
    }
  case EXPR_IS_TYPE:
    {
      const char* type = eval_type(node_get_child(expr, 1));
      if (var_query(type)){
        fatal_node(expr, "type '%s' in 'is' expression is a variable", type);
      }
      return ee_new("Bool", 
                    mem_asprintf("pack_bool(obj_klass(%s) == %s)",
                    eval_Value(node_get_child(expr, 0)),
                    cache_type(type)));
    }
  case EXPR_BLOCK:
    {
      fatal_push("in anonymous block");
      if (context_block != NULL){
        fatal_node(expr, "nested blocks are not implemented yet");
      }

      // Initialize context_block  
      context_block = mem_new(BlockContext);
      sbuf_init(&(context_block->sbuf_code), "");
      sarray_init(&(context_block->closure_names));
      sarray_init(&(context_block->closure_exprs));
      static int counter = 0; counter++;
      context_block->func_name = mem_asprintf("ripe_blk%d", counter);

      Node* param_list = node_get_node(expr, "param_list");
      Node* stmt_list = node_get_node(expr, "stmt_list");
      var_push();

      // Print out the header of the anonymous function
      sbuf_printf(&(context_block->sbuf_code), "static Value %s(Value __block",
                  context_block->func_name);
      for (int i = 0; i < node_num_children(param_list); i++){
        Node* param = node_get_child(param_list, i);
        const char* name = node_get_string(param, "name");
        const char* c_name = util_c_name(name);
        if (node_has_string(param, "array"))
          fatal_node(expr,
                     "array parameters for blocks are not implemented yet");
        const char* type = "?"; // TODO: Deal with type.
        var_add_local2(name, c_name, type, VAR_BLOCK_PARAM);
        sbuf_printf(&(context_block->sbuf_code), ", Value %s", c_name);
      }
      sbuf_printf(&(context_block->sbuf_code), ")\n");
      
      // Generate block code
      sbuf_printf(&(context_block->sbuf_code), "{\n");
      sbuf_printf(&(context_block->sbuf_code), 
                  "  Func* _c_data = obj_c_data(__block);\n");

      sbuf_printf(&(context_block->sbuf_code), 
                  "  stack_annot_push(\"anonymous function\");\n");
      sbuf_printf(&(context_block->sbuf_code), "%s", gen_block(stmt_list));
      sbuf_printf(&(context_block->sbuf_code), "}\n");

      // Now, print out the block function to WR_HEADER
      wr_print(WR_HEADER, "%s", context_block->sbuf_code.str);
      const char* result = mem_asprintf("block_to_val(%s, %d, %d",
                                        context_block->func_name,
                                        node_num_children(param_list),
                                        context_block->closure_names.size);
      for (uint i = 0; i < context_block->closure_names.size; i++){
        const char* evaluated = sarray_get_ptr(&(context_block->closure_exprs),
                                               i);
        result = mem_asprintf("%s, %s", result, evaluated);
      }
      result = mem_asprintf("%s)", result);

      // End EXPR_BLOCK
      var_pop();
      context_block = NULL;
      fatal_pop();
      return ee_new("Function", result);
    }
  default:
    assert_never();
  }
  return NULL;
}

