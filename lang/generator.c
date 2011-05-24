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

static const char* gen_block(Node* block) ATTR_WARN_UNUSED_RESULT;

///////////////////////////////////////////////////////////////////////////////
// LINE CONTROL
///////////////////////////////////////////////////////////////////////////////

static const char* line_filename;
static int line_min;
static int line_max;
static void traverse(Node* node)
{
  line_min = -1;
  line_max = -1;

  if (node->line != -1) {
    if (line_max == -1) {
      line_max = node->line;
      line_min = node->line;
    } else {
      if (line_max < node->line) line_max = node->line;
      if (line_min > node->line) line_min = node->line;
    }
  }
  for (int i = 0; i < node_num_children(node); i++){
    traverse(node_get_child(node, i));
  }
}

///////////////////////////////////////////////////////////////////////////////
// ERROR HANDLING
///////////////////////////////////////////////////////////////////////////////

// If node != NULL, attempt to query information about the location of the
// error.
static void fatal_node(Node* node, const char* format, ...)
{
  if (node != NULL) traverse(node);
  const char* error_numbers = NULL;
  if (line_min != -1){
    if (line_min == line_max){
      fatal_push("line %d", line_min);
    } else {
      fatal_push("lines %d-%d", line_min, line_max);
    }
  }

  va_list ap;
  va_start (ap, format);
  fatal_vthrow(format, ap);
}

///////////////////////////////////////////////////////////////////////////////
// CURRENT CONTEXT
///////////////////////////////////////////////////////////////////////////////

FuncInfo* context_fi = NULL;
ClassInfo* context_ci = NULL;
typedef struct {
  StringBuf sbuf_code;
  SArray closure_names;
  SArray closure_exprs;
  const char* func_name;
} BlockContext;
BlockContext* context_block = NULL;

///////////////////////////////////////////////////////////////////////////////
// BLOCK STUFF
///////////////////////////////////////////////////////////////////////////////

static const char* closure_add(const char* name, const char* evaluated)
{
  assert(context_block != NULL);
  
  for (int i = 0; i < context_block->closure_names.size; i++){
    const char* n = sarray_get_ptr(&(context_block->closure_names), i);
    if (strequal(name, n)){
      return mem_asprintf("_c_data->block_data[%d]", i);
    }
  }
  sarray_append_ptr(&(context_block->closure_names), (void*) name);
  sarray_append_ptr(&(context_block->closure_exprs), (void*) evaluated);
  return mem_asprintf("_c_data->block_data[%d]", 
                      context_block->closure_names.size - 1);
}

///////////////////////////////////////////////////////////////////////////////
// EVALUATING AST
///////////////////////////////////////////////////////////////////////////////
static const char* eval_expr(Node* expr);

static const char* eval_expr_list(Node* expr_list, bool first_comma)
{
  assert(expr_list->type == EXPR_LIST);
  StringBuf sb_temp;
  sbuf_init(&sb_temp, "");
  for (int i = 0; i < expr_list->children.size; i++){
    if (i == 0 and not first_comma){
      sbuf_printf(&sb_temp, "%s", eval_expr(node_get_child(expr_list, i)));
    } else {
      sbuf_printf(&sb_temp, ", %s", eval_expr(node_get_child(expr_list, i)));
    }
  }
  const char* result = mem_strdup(sb_temp.str);
  sbuf_deinit(&sb_temp);
  return result;
}

static const char* eval_expr_call(Node* expr, Node* arg_list)
{
  return mem_asprintf("func_call%d(%s %s)", 
                      node_num_children(arg_list),
                      eval_expr(expr),
                      eval_expr_list(arg_list, true));
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
      buf = mem_asprintf("%s%s", buf, eval_expr(arg));
    else
      buf = mem_asprintf("%s, %s", buf, eval_expr(arg));
  }
  if (is_vararg){
    if (min_params == 0)
      buf = mem_asprintf("%stuple_to_val(%d", buf, num_args - min_params);
    else
      buf = mem_asprintf("%s, tuple_to_val(%d", buf, num_args - min_params);

    for (int i = min_params; i < num_args; i++){
      Node* arg = node_get_child(arg_list, i);
      buf = mem_asprintf("%s, %s", buf, eval_expr(arg));
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
                      eval_expr(obj),
                      cache_dsym(method_name),
                      eval_expr_list(expr_list, true));
}

// Returns code for accessing index (if assign = NULL), or setting index
// when assign is of type expr.
static const char* eval_index(Node* self, Node* idx, Node* assign)
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
// I.e. Std.println would evaluate and return "Std.println", but (1+1).to_string
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

static const char* eval_expr(Node* expr)
{
  if (is_unary_op(expr))
    return mem_asprintf("%s(%s)",
                        unary_op_map(expr->type),
                        eval_expr(node_get_child(expr, 0)));
  if (is_binary_op(expr))
    return mem_asprintf("%s(%s, %s)",
                        binary_op_map(expr->type),
                        eval_expr(node_get_child(expr, 0)),
                        eval_expr(node_get_child(expr, 1)));

  switch(expr->type){
  case K_TRUE:
    return mem_strdup("VALUE_TRUE");
  case K_FALSE:
    return mem_strdup("VALUE_FALSE");
  case K_NIL:
    return mem_strdup("VALUE_NIL");
  case K_EOF:
    return mem_strdup("VALUE_EOF");
  case ID:
    if (context_block != NULL){
      // If it's a block param, then OK.
      if (var_query_kind(expr->text) == VAR_BLOCK_PARAM)
        return var_query_c_name(expr->text);
      else 
        return closure_add(expr->text, var_query_c_name(expr->text));
    } else return var_query_c_name(expr->text);
  case SYMBOL:
    return cache_dsym(expr->text + 1);
  case INT:
    return mem_asprintf("int64_to_val(%s)", expr->text);
  case DOUBLE:
    return mem_asprintf("double_to_val(%s)", expr->text);
  case STRING:
    {
      const char* str = expr->text;
      return mem_asprintf("string_to_val(\"%s\")", str);
    }
    break;
  case CHARACTER:
    {
      const char* str = expr->text;
      return mem_asprintf("int64_to_val(%d)", (int) str[1]);
    }
    break;
  case EXPR_ARRAY:
    {
      Node* expr_list = node_get_child(expr, 0);
      return mem_asprintf("array1_to_val2(%u %s)",
                          expr_list->children.size,
                          eval_expr_list(expr_list, true));
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
                                eval_expr(node_get_child(m, 0)),
                                eval_expr(node_get_child(m, 1)));
            break;
          case 1:
            is_set = true;
            args = mem_asprintf("%s, %s", args,
                                eval_expr(node_get_child(m, 0)));
            break;
          default:
            assert_never();
            break;
        }
      }

      if (is_map and is_set){
        fatal_node(expr, "invalid curly braces: is this a set or a map?");
      }
      if (is_map) return mem_asprintf("ht_new_map(%d%s)", num_ms, args);
      if (is_set) return mem_asprintf("ht_new_set(%d%s)", num_ms, args);
      assert_never();
    }
    break;
  case EXPR_INDEX:
    return eval_index(node_get_child(expr, 0), node_get_child(expr, 1), NULL);
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
          return eval_obj_call(left, field_name, args);
      }
      
      const char* s = eval_expr_as_id(callee);
      if (s != NULL){
        // Could be a tuple constructor.
        if (strequal(s, "tuple")){
          return mem_asprintf("tuple_to_val(%u %s)",
                              node_num_children(args),
                              eval_expr_list(args, true));
        }

        // If all of callee can be evaluated as an id, then it must be a static
        // call.
        return eval_static_call(s, args);
      }
      
      return eval_expr_call(callee, args);
    }
  case EXPR_RANGE_BOUNDED:
    {
      Node* left = node_get_child(expr, 0);
      Node* right = node_get_child(expr, 1);
      return mem_asprintf("range_to_val(RANGE_BOUNDED, "
                          "val_to_int64(%s), val_to_int64(%s))",
                          eval_expr(left),
                          eval_expr(right));
    }
  case EXPR_RANGE_BOUNDED_LEFT:
    return mem_asprintf("range_to_val(RANGE_BOUNDED_LEFT, "
                        "val_to_int64(%s), 0)",
                        eval_expr(node_get_child(expr, 0)));
  case EXPR_RANGE_BOUNDED_RIGHT:
    return mem_asprintf("range_to_val(RANGE_BOUNDED_RIGHT, "
                        "0, val_to_int64(%s))",
                        eval_expr(node_get_child(expr, 0)));
  case EXPR_RANGE_UNBOUNDED:
    return "range_to_val(RANGE_UNBOUNDED, 0, 0)";
  case EXPR_FIELD:
    {
      // Attempt to evaluate this field as a static symbol.
      Node* left = node_get_child(expr, 0);
      const char* s = eval_expr_as_id(left);
      if (s == NULL or var_query(s)){
        // Dynamic field.
        const char* field = node_get_string(expr, "name");

        return mem_asprintf("field_get(%s, %s)",
                            eval_expr(left),
                            cache_dsym(field));
      } else {
        // Could be a global variable.
        s = eval_expr_as_id(expr);

        if (context_block != NULL){
          return closure_add(s, var_query_c_name(s));
        } else return var_query_c_name(s);
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
      return mem_asprintf("field_get(__self, %s)", cache_dsym(name));
    }
  case C_CODE:
    {
      const char* str = util_trim_ends(expr->text);
      if (context_fi == NULL) return str;
      if (context_fi->type == METHOD
          or context_fi->type == CONSTRUCTOR
          or context_fi->type == VIRTUAL_GET
          or context_fi->type == VIRTUAL_SET){
        if (strchr(str, '@') != NULL and context_ci->type != CLASS_CDATA)
          fatal_node(expr, "@ in C code in a class that is not cdata");
        return util_replace(str, '@', "_c_data->");
      }
      if (strstr(expr->text, "return")){
        fatal_warn("careless return in C code may disrupt the stack (use RRETURN)");
      }
      return str;
    }
  case EXPR_IS_TYPE:
    {
      const char* type = eval_type(node_get_child(expr, 1));
      if (var_query(type)){
        fatal_node(expr, "type '%s' in 'is' expression is a variable", type);
      }
      return mem_asprintf("pack_bool(obj_klass(%s) == %s)",
                          eval_expr(node_get_child(expr, 0)),
                          cache_type(type));
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
      for (int i = 0; i < context_block->closure_names.size; i++){
        const char* evaluated = sarray_get_ptr(&(context_block->closure_exprs),
                                               i);
        result = mem_asprintf("%s, %s", result, evaluated);
      }
      result = mem_asprintf("%s)", result);

      // End EXPR_BLOCK
      var_pop();
      context_block = NULL;
      fatal_pop();
      return result;
    }
  default:
    assert_never();
  }
  return NULL;
}

static const char* gen_stmt_assign2(Node* lvalue, 
                                    Node* rvalue) ATTR_WARN_UNUSED_RESULT;
static const char* gen_stmt_assign2(Node* lvalue, Node* rvalue)
{
  StringBuf sb;
  sbuf_init(&sb, "");

  const char* right = eval_expr(rvalue);
  switch(lvalue->type){
  case ID:
    {
      const char* var_name = lvalue->text;
      if (var_query(var_name)){
        sbuf_printf(&sb, "  %s = %s;\n", var_query_c_name(var_name), right);
      } else {
        // Register the variable (untyped)
        const char* c_name = util_c_name(var_name);
        var_add_local(var_name, c_name, "?");
        sbuf_printf(&sb, "  Value %s = %s;\n", c_name, right);
      }
    }
    break;
  case EXPR_TYPED_ID:
    {
      const char* var_name = node_get_string(lvalue, "name");
      const char* c_name = util_c_name(var_name);
      const char* type = eval_type(node_get_node(lvalue, "type"));
      var_add_local(var_name, c_name, type);
      sbuf_printf(&sb, "  Value %s = %s;\n", c_name, right);
    }
    break;
  case EXPR_INDEX:
    sbuf_printf(&sb, "  %s;\n",
                eval_index(node_get_child(lvalue, 0),
                           node_get_child(lvalue, 1),
                           rvalue));
    break;
  case EXPR_FIELD:
    sbuf_printf(&sb, "  field_set(%s, %s, %s);\n",
                eval_expr(node_get_child(lvalue, 0)),
                cache_dsym(node_get_string(lvalue, "name")),
                right);
    break;
  case EXPR_AT_VAR:
    sbuf_printf(&sb, "  field_set(__self, %s, %s);\n",
             cache_dsym(node_get_string(lvalue, "name")),
             right);
    break;
  default:
    assert_never();
  }
  
  return sb.str;
}

static const char* gen_stmt_assign(Node* left, 
                                   Node* right) ATTR_WARN_UNUSED_RESULT;
static const char* gen_stmt_assign(Node* left, Node* right)
{
  StringBuf sb;
  sbuf_init(&sb, "");

  if (node_num_children(left) == 1){
    left = node_get_child(left, 0);
    sbuf_printf(&sb, "%s", gen_stmt_assign2(left, right));
  } else {
    static int counter = 0;
    counter++;
    char* tmp_variable = mem_asprintf("_tmp_assign_%d", counter);

    Node* id_tmp = node_new_id(tmp_variable);
    sbuf_printf(&sb, "%s", gen_stmt_assign2(id_tmp, right));

    for (int i = 0; i < node_num_children(left); i++){
      Node* l = node_get_child(left, i);
      sbuf_printf(&sb, "%s", 
                  gen_stmt_assign2(l, node_new_expr_index1(id_tmp, 
                                                           node_new_int(i+1))));
    }
  }
  
  return sb.str;
}

static const char* gen_stmt(Node* stmt)
{
  StringBuf sb;
  sbuf_init(&sb, "");

  // Traverse the statement
  traverse(stmt);
  if (line_min != -1){
    sbuf_printf(&sb, "#line %d \"%s\"\n", line_min, line_filename);
  }

  switch(stmt->type){
  case STMT_EXPR:
    {
      Node* expr = node_get_child(stmt, 0);
      if (expr->type == C_CODE) {
        sbuf_printf(&sb, "  %s\n", eval_expr(expr));
      } else {
        sbuf_printf(&sb, "  %s;\n", eval_expr(expr));
      }
    }
    break;
  case STMT_RETURN:
    if (context_fi->type == CONSTRUCTOR){
      fatal_node(stmt, "return not allowed in a constructor");
    }
    sbuf_printf(&sb, "  RRETURN(%s);\n",
                eval_expr(node_get_child(stmt, 0)));
    break;
  case STMT_LOOP:
    {
      const char* lbl_break = stacker_label();
      const char* lbl_continue = stacker_label();
      stacker_push(STACKER_LOOP, lbl_break, lbl_continue);
      
      sbuf_printf(&sb, "  for(;;){\n");
      sbuf_printf(&sb, " %s:;\n", lbl_continue);
      sbuf_printf(&sb, "%s", gen_block(node_get_child(stmt, 0)));
      sbuf_printf(&sb, "  }\n");
      sbuf_printf(&sb, " %s:;\n", lbl_break);

      stacker_pop();
    }
    break;
  case STMT_IF:
    sbuf_printf(&sb, "  if (%s == VALUE_TRUE) {\n",
             eval_expr(node_get_node(stmt, "expr")));
    sbuf_printf(&sb, "%s", gen_block(node_get_node(stmt, "block")));
    sbuf_printf(&sb, "  }\n");
    break;
  case STMT_ELIF:
    sbuf_printf(&sb, "  else if (%s == VALUE_TRUE) {\n",
             eval_expr(node_get_node(stmt, "expr")));
    sbuf_printf(&sb, "%s", gen_block(node_get_node(stmt, "block")));
    sbuf_printf(&sb, "  }\n");
    break;
  case STMT_ELSE:
    sbuf_printf(&sb, "  else {\n");
    sbuf_printf(&sb, "%s", gen_block(node_get_node(stmt, "block")));
    sbuf_printf(&sb, "  }\n");
    break;
  case STMT_BREAK:
    {
      int num = 1;
      if (node_has_string(stmt, "num")) num = atoi(node_get_string(stmt, "num"));
      sbuf_printf(&sb, "  goto %s;\n", stacker_break(num));
    }
    break;
  case STMT_CONTINUE:
    {
      int num = 1;
      if (node_has_string(stmt, "num")) num = atoi(node_get_string(stmt, "num"));
      sbuf_printf(&sb, "  goto %s;\n", stacker_continue(num));
    }
    break;
  case STMT_ASSIGN:
    sbuf_printf(&sb, "%s", gen_stmt_assign(node_get_child(stmt, 0), 
                                           node_get_child(stmt, 1)));
    break;
  case STMT_FOR:
    {
      static int iterator_counter = 0;
      iterator_counter++;
      Node* expr = node_get_child(stmt, 1);
      Node* lvalue_list = node_get_child(stmt, 0);

      // Outside scope begin
      var_push();
      sbuf_printf(&sb, "  {\n");
      
      // _iteratorX = expr.get_iter()
      const char* rip_iterator = mem_asprintf("_iterator%d", iterator_counter);
      Node* id_iterator = node_new_id(rip_iterator);
      sbuf_printf(&sb, "%s", 
                  gen_stmt_assign2(id_iterator, 
                                   node_new_field_call(expr, "get_iter", 0)));

      // loop
      const char* lbl_break = stacker_label();
      const char* lbl_continue = stacker_label();
      sbuf_printf(&sb, "  for(;;){");
      sbuf_printf(&sb, " %s:;\n", lbl_continue);
     
      // _iterator_tempX = _iteratorX.iter()
      Node* iterator_call = node_new_field_call(id_iterator, "iter", 0);
      Node* id_temp = node_new_id(mem_asprintf("_iterator_temp%d",
                                  iterator_counter));
      sbuf_printf(&sb, "%s", gen_stmt_assign2(id_temp, iterator_call));

      // if _iterator_tempX == eof break
      sbuf_printf(&sb, "  if (%s == VALUE_EOF) break;\n", eval_expr(id_temp));
      sbuf_printf(&sb, "%s", gen_stmt_assign(lvalue_list, id_temp));

      // Write out code
      stacker_push(STACKER_FOR, lbl_break, lbl_continue);
      sbuf_printf(&sb, "%s", gen_block(node_get_child(stmt, 2)));
      stacker_pop();

      sbuf_printf(&sb, "  }\n"); // end loop
      sbuf_printf(&sb, " %s:;\n", lbl_break);

      // Outside scope end
      sbuf_printf(&sb, "  }\n");
      var_pop();
    }
    break;
  case STMT_SWITCH:
    {
      static int switch_counter = 0;
      const char* c_expr_value = mem_asprintf("_switch_expr%d", switch_counter);
      Node* expr = node_get_child(stmt, 0);
      Node* case_list = node_get_child(stmt, 1);

      sbuf_printf(&sb, "  {\n");
      sbuf_printf(&sb, "  Value %s = %s;\n", c_expr_value,
                                                     eval_expr(expr));
      int num_cases = node_num_children(case_list);
      for (int i = 0; i < num_cases; i++){
        Node* node_case = node_get_child(case_list, i);
        Node* node_case_expr = node_get_child(node_case, 0);
        Node* block = node_get_child(node_case, 1);
        const char* word = "else if";
        if (i == 0) word = "if";
        sbuf_printf(&sb, "  %s (op_equal(%s, %s) == VALUE_TRUE) {\n",
                    word, c_expr_value, eval_expr(node_case_expr));
        sbuf_printf(&sb, "%s", gen_block(block));
        sbuf_printf(&sb, "  }\n");
      }
      if (node_has_node(stmt, "else")){
        sbuf_printf(&sb, "  else {\n");
        sbuf_printf(&sb, "%s", gen_block(node_get_node(stmt, "else")));
        sbuf_printf(&sb, "  }\n");
      }
      sbuf_printf(&sb, "  }\n");
    }
    break;
  case STMT_PASS:
    break;
  case STMT_RAISE:
    sbuf_printf(&sb, "  exc_raise_object(%s);\n",
             eval_expr(node_get_child(stmt, 0)));
    break;
  default:
    fatal_node(stmt, "invalid statement type %d", stmt->type);
  }
  
  return sb.str;
}

static const char* gen_try_stuff(Node* try_stmt, 
                                 Node* other_stmt) ATTR_WARN_UNUSED_RESULT;
static const char* gen_try_stuff(Node* try_stmt, Node* other_stmt)
{
  assert(try_stmt->type == STMT_TRY);

  StringBuf sb;
  sbuf_init(&sb, "");

  switch(other_stmt->type){
  case STMT_CATCH:
    //   try ...
    sbuf_printf(&sb, "  if (setjmp(exc_jb) == 0){\n");
    if (node_has_node(other_stmt, "type")){
      sbuf_printf(&sb, "    stack_push_catch(%s);\n",
               cache_type(
                 eval_type(
                   node_get_node(other_stmt, "type")
                 )
               ));
    } else {
      sbuf_printf(&sb, "    stack_push_catch_all();\n");
    }

    // Generate try code
    stacker_push(STACKER_TRY, NULL, NULL);
    sbuf_printf(&sb, "%s", gen_block(node_get_node(try_stmt, "block")));
    stacker_pop();
    
    sbuf_printf(&sb, "    stack_pop()\n;");

    //   catch ...
    sbuf_printf(&sb, "  } else {\n");
    var_push();

    if (node_has_node(other_stmt, "type")
        and node_has_string(other_stmt, "name")){
      const char* ripe_name = node_get_string(other_stmt, "name");
      const char* c_name = util_c_name(ripe_name);
      sbuf_printf(&sb, "  Value %s = exc_obj;\n", c_name);
      var_add_local(ripe_name, c_name,
                    eval_type(node_get_node(other_stmt, "type")));
    }

    // Generate catch code
    stacker_push(STACKER_CATCH, NULL, NULL);
    sbuf_printf(&sb, "%s", gen_block(node_get_node(other_stmt, "block")));
    stacker_pop();

    var_pop();
    sbuf_printf(&sb, "  }\n");
    break;
  case STMT_FINALLY:
    sbuf_printf(&sb, "  if (setjmp(exc_jb) == 0){\n");
    sbuf_printf(&sb, "    stack_push_finally();\n");
    stacker_push(STACKER_TRY, NULL, NULL);
    sbuf_printf(&sb, "%s", gen_block(node_get_node(try_stmt, "block")));
    stacker_pop();
    sbuf_printf(&sb, "    stack_pop()\n;");
    sbuf_printf(&sb, "  }\n");
    stacker_push(STACKER_FINALLY, NULL, NULL);
    sbuf_printf(&sb, "%s", gen_block(node_get_node(other_stmt, "block")));
    stacker_pop();
    sbuf_printf(&sb, "  if (stack_unwinding == true)\n");
    sbuf_printf(&sb, "    stack_continue_unwinding();\n");
    break;
  default:
    fatal_throw("invalid statement following try block");
  }
  return sb.str;
}

static const char* gen_block(Node* block)
{
  var_push(); // Each block is a scope.

  StringBuf sb;
  sbuf_init(&sb, "");

  int prev_stmt_type = 0;
  uint i = 0;
  while (i < block->children.size){
    Node* stmt = node_get_child(block, i);

    // Check that ELSE and ELIF follow IF and ELIF
    if (stmt->type == STMT_ELSE or stmt->type == STMT_ELIF){
      if (prev_stmt_type != STMT_IF and
          prev_stmt_type != STMT_ELIF){
        fatal_node(stmt, "statement must follow if or elif");
      }
    }

    // These are handled specially because of the nested structure.
    if (stmt->type == STMT_TRY){
      // Find the extent of the try/catch blocks
      uint itry = i;
      uint ilast = i;
      for(;;){
        i++;
        if (i >= block->children.size) break;
        stmt = node_get_child(block, i);
        if (stmt->type != STMT_CATCH and stmt->type != STMT_FINALLY) break;
        ilast = i;
      }
      if (itry == ilast) fatal_throw("try by itself");

      if (ilast == itry+1){
        sbuf_printf(&sb, "%s", gen_try_stuff(node_get_child(block, itry),
                                            node_get_child(block, ilast)));
        continue;
      }

      // try                       try
      //   BLOCK 1                   try
      // catch                         BLOCK 1
      //   BLOCK 2             =     catch
      // catch                         BLOCK 2
      //   BLOCK 3                 catch
      //                             BLOCK 3
      // The following code generates this structure:
      Node* try = node_get_child(block, itry);
      Node* other = node_get_child(block, itry+1);
      int inext = itry+2;
      for(;;){
        // Combine try_block + other into try_block:
        Node* new_block = node_new(STMT_LIST);
        node_add_child(new_block, try);
        node_add_child(new_block, other);
        // new_block is   { try
        //                    ...
        //                  other
        //                    ... }

        Node* new_try = node_new(STMT_TRY);
        node_set_node(new_try, "block", new_block);
        // new_try is   try
        //                try
        //                  ...
        //                other
        //                  ...
        if (inext == ilast){
          sbuf_printf(&sb, gen_try_stuff(new_try, node_get_child(block, ilast)));
          break;
        } else {
          other = node_get_child(block, inext);
          try = new_try;
          inext++;
        }
      } // end combining for
      continue;
    } // endif STMT_TRY

    prev_stmt_type = stmt->type;
    
    // If in a block, then must return result of latest expression or NIL
    if (context_block != NULL and i == block->children.size - 1){
      if (stmt->type == STMT_EXPR){
        sbuf_printf(&sb, "  RRETURN(\n%s);\n", 
                    eval_expr(node_get_child(stmt, 0)));
      } else {
        sbuf_printf(&sb, "%s", gen_stmt(stmt));
        sbuf_printf(&sb, "  RRETURN(VALUE_NIL);\n");
      }
    } else {
      sbuf_printf(&sb, "%s", gen_stmt(stmt));
    }
    
    i++;
  }

  var_pop();
  return sb.str;
}

static void register_locals(const char* name)
{
  FuncInfo* fi = stran_get_function(name); assert(fi != NULL);

  for (int i = 0; i < fi->num_params; i++){
    const char* var_type = fi->param_types[i];
    if (strequal(var_type, "*")) var_type = "Tuple";
    var_add_local(fi->param_names[i],
                  util_c_name(fi->param_names[i]),
                  var_type);
  }
}

// Generate all the statements, and maybe return VALUE_NIL at the end.
static void gen_code(Node* n, const char* name)
{
  // This function is only called by gen_function, gen_method or gen_constructor.
  // Do not call it from anywhere else (use gen_block for that).
  context_fi = stran_get_function(name);
  fatal_push("while compiling function '%s'", name);

  wr_print(WR_CODE, "%s\n{\n", util_signature(name));
  wr_print(WR_CODE, "  stack_annot_push(\"%s\");\n", name);
  Node* stmt_list = node_get_node(n, "stmt_list");
  stacker_init();

  var_push();
  register_locals(name);
  switch(context_fi->type){
  case CONSTRUCTOR:
    var_add_local("self", "__self", context_ci->ripe_name);
    if (context_ci->type == CLASS_CDATA) {
      wr_print(WR_CODE, "  %s* _c_data;\n", context_ci->typedef_name);
      wr_print(WR_CODE, "  Value __self = obj_new(%s, (void**) &_c_data);\n",
               context_ci->c_name);
    } else {
      wr_print(WR_CODE, "  Value __self = obj_new2(%s);\n",
               context_ci->c_name);
    }
    break;
  case VIRTUAL_GET:
  case VIRTUAL_SET:
  case METHOD:
    if (context_ci->type == CLASS_CDATA) {
      wr_print(WR_CODE, "  %s* _c_data = obj_c_data(__self);\n",
               context_ci->typedef_name);
    }
    break;
  default:
    /* nothing */
    break;
  }
  wr_print(WR_CODE, "%s", gen_block(stmt_list));
  var_pop();

  // If last statement is of type STMT_RETURN, no need for another return.
  if (context_fi->type != CONSTRUCTOR){
    if (stmt_list->children.size == 0
         or
      node_get_child(stmt_list, stmt_list->children.size-1)->type != STMT_RETURN)
    wr_print(WR_CODE, "  RRETURN(VALUE_NIL);\n");
  } else {
    // If this is a constructor, remember to return the new object!
    wr_print(WR_CODE, "  RRETURN(__self);\n");
  }

  wr_print(WR_CODE, "}\n");

  assert(stacker_size() == 0);
  context_fi = NULL;
  fatal_pop();
}

static void gen_function(Node* n, const char* name)
{
  gen_code(n, name);
}

static void gen_constructor(Node* n, const char* name,
                            const char* class_name)
{
  context_ci = stran_get_class(class_name); assert(context_ci != NULL);

  gen_code(n, mem_asprintf("%s.%s", class_name, name));

  context_ci = NULL;
}

static void gen_method(Node* n, const char* name, const char* class_name,
                       FunctionType type)
{
  context_ci = stran_get_class(class_name); assert(context_ci != NULL);

  const char* real_name = name;
  if (type == VIRTUAL_GET) real_name = mem_asprintf("get_%s", name);
  if (type == VIRTUAL_SET) real_name = mem_asprintf("set_%s", name);
  gen_code(n, mem_asprintf("%s.%s", class_name, real_name));

  context_ci = NULL;
}

static void gen_var(Node* n, const char* name)
{
  GlobalInfo* gi = stran_get_global(name);
  assert(gi != NULL);

  if (node_has_node(n, "value")){
    Node* expr = node_get_node(n, "value");
    wr_print(WR_INIT3, "  %s = %s;\n", gi->c_name, eval_expr(expr));
  }
}

void generate(Node* ast, const char* filename)
{
  line_filename = filename;

  Aster aster;
  aster_init(&aster);
  aster.cb_function = gen_function;
  aster.cb_method = gen_method;
  aster.cb_constructor = gen_constructor;
  aster.cb_var = gen_var;
  aster_process(ast, &aster);
}
