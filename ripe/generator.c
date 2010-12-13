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
#include "clib/stringbuf.h"
#include "clib/dict.h"
#include <setjmp.h>

///////////////////////////////////////////////////////////////////////////////
// DYNAMIC SYMBOL, STATIC SYMBOL AND TYPE TABLES
///////////////////////////////////////////////////////////////////////////////

static Dict tbl_ssym; // symbol_name -> string name of C variable of type Value
static Dict tbl_dsym; // symbol name -> integer 0...
static Dict tbl_types; // type name -> string name of C variable of type Klass*

static void tables_init()
{
  dict_init(&tbl_ssym, sizeof(char*), sizeof(char*),
            dict_hash_string, dict_equal_string);
  dict_init(&tbl_dsym, sizeof(char*), sizeof(char*),
            dict_hash_string, dict_equal_string);
  dict_init(&tbl_types, sizeof(char*), sizeof(char*),
            dict_hash_string, dict_equal_string);
}

// Returns the name of the global static C variable of type Klass* that
// corresponds to that typename.
static const char* tbl_get_type(const char* type)
{
  char* klassp_c_var;
  if (dict_query(&tbl_types, &type, &klassp_c_var))
    return klassp_c_var;

  static uint64 counter = 0;
  counter++;
  klassp_c_var = mem_asprintf("_klass%"PRIu64"_%s",
                              counter,
                              util_escape(type));
  sbuf_printf(sb_header, "static Klass* %s;\n",
              klassp_c_var);
  sbuf_printf(sb_init2, "  %s = klass_get(dsym_get(\"%s\"));\n",
              klassp_c_var, type);
  dict_set(&tbl_types, &type, &klassp_c_var);
  return klassp_c_var;
}

// Returns the name of the global static C variable of type Value that
// corresponds to that symbol.
static const char* tbl_get_ssym(const char* symbol)
{
  char* ssym_c_var;
  if (dict_query(&tbl_ssym, &symbol, &ssym_c_var))
    return ssym_c_var;

  static uint64 counter = 0;
  counter++;

  ssym_c_var = mem_asprintf("_ssym%"PRIu64"_%s",
                            counter,
                            util_escape(symbol));
  sbuf_printf(sb_header, "static Value %s;\n", ssym_c_var);
  sbuf_printf(sb_init2, "  %s = ssym_get(\"%s\");\n",
                         ssym_c_var, symbol);
  dict_set(&tbl_ssym, &symbol, &ssym_c_var);  return ssym_c_var;
}

// Returns the name of the global static C variable of type Value that
// corresponds to that symbol.
static const char* tbl_get_dsym(const char* symbol)
{
  char* dsym_c_var;
  if (dict_query(&tbl_dsym, &symbol, &dsym_c_var))
    return dsym_c_var;

  static uint64 counter = 0;
  counter++;

  dsym_c_var = mem_asprintf("_dsym%"PRIu64"_%s",
                            counter,
                            util_escape(symbol));
  sbuf_printf(sb_header, "static Value %s;\n", dsym_c_var);
  sbuf_printf(sb_init2, "  %s = dsym_get(\"%s\");\n",
                         dsym_c_var, symbol);
  dict_set(&tbl_dsym, &symbol, &dsym_c_var);
  return dsym_c_var;
}

///////////////////////////////////////////////////////////////////////////////
// CURRENT CONTEXT
///////////////////////////////////////////////////////////////////////////////

typedef enum {
  CLASS_VIRTUAL_OBJECT,
  CLASS_CDATA_OBJECT,
  CLASS_FIELD_OBJECT
} ClassType;

int context;
#define CONTEXT_NONE 1
#define CONTEXT_CLASS 2
const char* context_class_name;
const char* context_class_c_name;
const char* context_class_typedef;
ClassType context_class_type;
Dict* context_class_dict;
int context2;
#define CONTEXT_FUNC        1
#define CONTEXT_CONSTRUCTOR 2
#define CONTEXT_METHOD      3
const char* context2_method_value_name;

///////////////////////////////////////////////////////////////////////////////
// EVALUATING AST
///////////////////////////////////////////////////////////////////////////////
static uint dowhile_semaphore;
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

static const char* eval_static_call(const char* ssym, Node* arg_list)
{
  TyperRecord* tr = typer_query(ssym);
  // In case this is a call to a method '#' must be replaced with '.' from here
  // on.
  ssym = util_replace(ssym, '#', ".");

  assert(tr != NULL);
  int num_args = node_num_children(arg_list);
  int min_params = num_args;
  int num_params = tr->num_params;
  bool is_vararg = false;

  // Check if vararg
  if (tr->num_params > 0 and
      tr->param_types[tr->num_params-1] != NULL
       and strequal("*", tr->param_types[tr->num_params-1])){
    is_vararg = true;
    min_params = tr->num_params - 1;
    if (num_args < min_params)
      err_node(arg_list, "'%s' called with %d arguments but expect at least %d",
               ssym, num_args, min_params);
  } else {
    if (tr->num_params != num_args)
      err_node(arg_list, "'%s' called with %d arguments but expect %d", ssym,
               num_args, tr->num_params);
  }
  const char* buf = mem_asprintf("func_call%d(%s", num_params,
                                 tbl_get_ssym(ssym));
  for (int i = 0; i < min_params; i++){
    const char* param_type = tr->param_types[i];
    Node* arg = node_get_child(arg_list, i);
    const char* arg_type = typer_infer(arg);

    if (typer_needs_check(param_type, arg_type)){
      buf = mem_asprintf("%s, obj_verify_assign(%s, %s)", buf, eval_expr(arg),
                         tbl_get_type(param_type));
    } else {
      buf = mem_asprintf("%s, %s", buf, eval_expr(arg));
    }
  }
  if (is_vararg){
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
    const char* type = typer_infer(obj);
    if (type == NULL) {
      return mem_asprintf("method_call%d(%s, %s %s)",
                          node_num_children(expr_list),
                          eval_expr(obj),
                          tbl_get_dsym(method_name),
                          eval_expr_list(expr_list, true));
    } else {
      Node* arg_list = node_new_expr_list();
      node_add_child(arg_list, obj);
      node_extend_children(arg_list, expr_list);
      return eval_static_call(mem_asprintf("%s#%s", type, method_name),
                              arg_list);
    }
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
      return expr->text;
    case EXPR_FIELD:
      {
        Node* parent = node_get_child(expr, 0);
        Node* field = node_get_child(expr, 1);
        assert(field->type == ID);
        const char* s = eval_expr_as_id(parent);
        if (s != NULL) return mem_asprintf("%s.%s", s, field->text);
        else return NULL;
      }
    default:
      // Anything other than ID or EXPR_FIELD means that it cannot be a
      // static symbol.
      return NULL;
  }
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
      {
        const char* local = query_local(expr->text);
        if (local == NULL) return tbl_get_ssym(expr->text);
        return mem_strdup(local);
      }
    case SYMBOL:
      return tbl_get_dsym(expr->text + 1);
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
    case EXPR_INDEX:
      return eval_index(node_get_child(expr, 0), node_get_child(expr, 1), NULL);
    case EXPR_FIELD_CALL:
      {
        Node* parent = node_get_child(expr, 0);
        const char* field = node_get_string(expr, "name");
        Node* arg_list = node_get_child(expr, 1);

        // Attempt to evaluate parent as a static symbol.
        const char* s = eval_expr_as_id(parent);
        if (s == NULL or (query_local(s) != NULL)){
          // Dynamic call
          return eval_obj_call(parent, field, arg_list);
        }
        // Static call
        return eval_static_call(mem_asprintf("%s.%s", s, field), arg_list);
      }
    case EXPR_ID_CALL:
      {
        Node* left = node_get_child(expr, 0);
        Node* arg_list = node_get_child(expr, 1);
        assert(left->type == ID);
        if (query_local(left->text))
          err_node(expr, "variable '%s' called as a function", left->text);
        if (strequal(left->text, "tuple")){
          return mem_asprintf(
                   "tuple_to_val(%u %s)",
                   node_num_children(arg_list),
                   eval_expr_list(arg_list, true)
                 );
        }
        // TODO: Deprecate call_func and allow variables to be called as
        // functions.
        if (strequal(left->text, "call_func")){
          return mem_asprintf("func_call%u(%s)",
                               node_num_children(arg_list) - 1,
                               eval_expr_list(arg_list, false));
        }

        // At this point it must be a static call.
        return eval_static_call(left->text, arg_list);
      }
      break;
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
      return mem_strdup("range_to_val(RANGE_UNBOUNDED, 0, 0)");

    case EXPR_FIELD:
      {
        // Attempt to evaluate this field as a static symbol.
        Node* left = node_get_child(expr, 0);
        const char* s = eval_expr_as_id(left);
        if (s == NULL or query_local(s) != NULL){
          // Dynamic field.
          Node* parent = node_get_child(expr, 0);
          Node* field = node_get_child(expr, 1);

          return mem_asprintf("field_get(%s, %s)",
                              eval_expr(parent),
                              tbl_get_dsym(field->text));
        } else {
          // Could be a global variable.
          s = eval_expr_as_id(expr);
          const char* global_var = query_local(s);
          if (global_var != NULL){
            return global_var;
          }

          return tbl_get_ssym(s);
        }
      }
      break;
    case EXPR_AT_VAR:
      {
        const char* name = node_get_string(expr, "name");
        if (context != CONTEXT_CLASS){
          err_node(expr, "'@%s' in something that's not a class", name);
        }
        if (context_class_type != CLASS_FIELD_OBJECT){
          err_node(expr, "'@%s' in a class that's not a field class", name);
        }
        char* field_int_var;
        if (not dict_query(context_class_dict, &name, &field_int_var)){
          static int counter = 0; counter++;
          field_int_var = mem_asprintf("field_int_%d", counter);
          sbuf_printf(sb_header, "static int %s;\n", field_int_var);
          sbuf_printf(sb_init2, "  %s = klass_get_field_int("
                      "%s, dsym_get(\"%s\"));\n", field_int_var,
                      context_class_c_name, name);
          dict_set(context_class_dict, &name, &field_int_var);
        }
        return mem_asprintf("_c_data[%s]", field_int_var);
      }
    case C_CODE:
      {
        const char* str = util_trim_ends(expr->text);
        if (context2 == CONTEXT_METHOD or context2 == CONTEXT_CONSTRUCTOR){
          if (strchr(str, '@') != NULL and context_class_type != CLASS_CDATA_OBJECT)
            err_node(expr, "@ in C code in a class that is not cdata");
          return util_replace(str, '@', "_c_data->");
        }
        return str;
      }
    case EXPR_IS_TYPE:
      {
        const char* type = eval_type(node_get_child(expr, 1));
        if (query_local(type)){
          err_node(expr, "type '%s' in 'is' expression is a variable", type);
        }
        return mem_asprintf("pack_bool(obj_klass(%s) == %s)",
                            eval_expr(node_get_child(expr, 0)),
                            tbl_get_type(type));
      }
    default:
      assert_never();
  }
  return NULL;
}

static void gen_stmt_expr(Node* stmt)
{
  Node* expr = node_get_child(stmt, 0);
  if (expr->type == EXPR_ID_CALL or expr->type == EXPR_FIELD_CALL){
    sbuf_printf(sb_contents, "  %s;\n", eval_expr(expr));
  } else if (expr->type == C_CODE) {
    sbuf_printf(sb_contents, "  %s\n", eval_expr(expr));
  } else {
    err_node(stmt, "invalid expression in an expression statement");
  }
}

static void gen_stmtlist(Node* stmtlist);
static void gen_stmtlist_no_locals(Node* stmtlist);

static void gen_stmt_assign2(Node* lvalue, Node* rvalue)
{
  const char* right = eval_expr(rvalue);
  const char* rtype = typer_infer(rvalue);
  switch(lvalue->type){
    case ID:
      {
        Variable* var = query_local_full(lvalue->text);
        if (var == NULL){
          // Register the variable (untyped)
          sbuf_printf(sb_contents, "  Value %s = %s;\n",
                      register_local(lvalue->text, NULL), right);
          break;
        }

        if (not typer_needs_check(var->type, rtype)){
          sbuf_printf(sb_contents, "  %s = %s;\n", var->c_name, right);
        } else {
          sbuf_printf(sb_contents, "  %s = obj_verify_assign(%s, %s);\n",
                      var->c_name, right, tbl_get_type(var->type));
        }
      }
      break;
    case EXPR_TYPED_ID:
      {
        const char* ripe_name = node_get_string(lvalue, "name");
        const char* type = eval_type(node_get_child(lvalue, 0));
        Variable* var = query_local_full(ripe_name);
        if (var != NULL){
          err_node(lvalue, "variable '%s' already defined", ripe_name);
        }
        const char* c_name = register_local(ripe_name, type);
        if (not typer_needs_check(type, rtype)){
          sbuf_printf(sb_contents, "  Value %s = %s;\n",
                      c_name, right);
        } else {
          sbuf_printf(sb_contents, "  Value %s = obj_verify_assign(%s, %s);\n",
                      c_name, right, tbl_get_type(type));
        }
      }
      break;
    case EXPR_INDEX:
      sbuf_printf(sb_contents, "  %s;\n",
                  eval_index(node_get_child(lvalue, 0),
                             node_get_child(lvalue, 1),
                             rvalue));
      break;
    case EXPR_FIELD:
      sbuf_printf(sb_contents, "  field_set(%s, %s, %s);\n",
                  eval_expr(node_get_child(lvalue, 0)),
                  tbl_get_dsym(node_get_child(lvalue, 1)->text),
                  right);
      break;
    case EXPR_AT_VAR:
      sbuf_printf(sb_contents, "  %s = %s;\n",
                  eval_expr(lvalue),
                  right);
      break;
    default:
      assert_never();
  }
}

static void gen_stmt_assign(Node* left, Node* right)
{
  if (node_num_children(left) == 1){
    left = node_get_child(left, 0);
    gen_stmt_assign2(left, right);
  } else {
    static int counter = 0;
    counter++;
    char* tmp_variable = mem_asprintf("_tmp_assign_%d", counter);

    Node* id_tmp = node_new_id(tmp_variable);
    gen_stmt_assign2(id_tmp,
                     right);

    for (int i = 0; i < node_num_children(left); i++){
      Node* l = node_get_child(left, i);
      gen_stmt_assign2(l, node_new_expr_index1(id_tmp, node_new_int(i+1) ));
    }
  }
}

static void gen_stmt(Node* stmt)
{
  switch(stmt->type){
    case STMT_EXPR:
      gen_stmt_expr(stmt);
      break;
    case STMT_RETURN:
      if (context2 == CONTEXT_CONSTRUCTOR){
        err_node(stmt, "return not allowed in a constructor");
      }
      sbuf_printf(sb_contents, "  return %s;\n",
                                eval_expr(node_get_child(stmt, 0)));

      break;
    case STMT_LOOP:
      {
        sbuf_printf(sb_contents, "  for(;;){\n");
        dowhile_semaphore++;
        gen_stmtlist(node_get_child(stmt, 0));
        dowhile_semaphore--;
        sbuf_printf(sb_contents, "  }\n");
      }
      break;
    case STMT_IF:
      {
        sbuf_printf(sb_contents, "  if (%s == VALUE_TRUE) {\n",
                                  eval_expr(node_get_child(stmt, 0)));
        gen_stmtlist(node_get_child(stmt, 1));
        sbuf_printf(sb_contents, "  }\n");
      }
      break;
    case STMT_ELIF:
      {
        sbuf_printf(sb_contents, "  else if (%s == VALUE_TRUE) {\n",
                                  eval_expr(node_get_child(stmt, 0)));
        gen_stmtlist(node_get_child(stmt, 1));
        sbuf_printf(sb_contents, "  }\n");
      }
      break;
    case STMT_ELSE:
      {
        sbuf_printf(sb_contents, "  else {\n");
        gen_stmtlist(node_get_child(stmt, 0));
        sbuf_printf(sb_contents, "  }\n");
      }
      break;
    case STMT_WHILE:
      {
        sbuf_printf(sb_contents, "  while (%s == VALUE_TRUE) {\n",
                                  eval_expr(node_get_child(stmt, 0)));
        dowhile_semaphore++;
        gen_stmtlist(node_get_child(stmt, 1));
        dowhile_semaphore--;
        sbuf_printf(sb_contents, "  }\n");
      }
      break;
    case STMT_BREAK:
      if (dowhile_semaphore == 0){
        err_node(stmt, "break outside a loop");
      }
      sbuf_printf(sb_contents, "  break;\n");
      break;
    case STMT_CONTINUE:
      if (dowhile_semaphore == 0){
        err_node(stmt, "continue outside a loop");
      }
      sbuf_printf(sb_contents, "  continue;\n");
      break;
    case STMT_ASSIGN:
      gen_stmt_assign(node_get_child(stmt, 0), node_get_child(stmt, 1));
      break;
    case STMT_FOR:
      {
        static int iterator_counter = 0;
        iterator_counter++;
        const char* rip_iterator = mem_asprintf("_iterator%d", iterator_counter);
        Node* id_iterator = node_new_id(rip_iterator);
        Node* expr = node_get_child(stmt, 1);
        Node* lvalue_list = node_get_child(stmt, 0);

        push_locals();
        gen_stmt_assign2(id_iterator, node_new_field_call(expr, "get_iter", 0));
        Node* iterator_call = node_new_field_call(id_iterator, "iter", 0);

        sbuf_printf(sb_contents, "  for(;;){");
        Node* id_temp = node_new_id(mem_asprintf("_iterator_temp%d",
                                              iterator_counter));
        gen_stmt_assign2(id_temp, iterator_call);
        sbuf_printf(sb_contents, "  if (%s == VALUE_EOF) break;\n", eval_expr(id_temp));

        if (node_num_children(lvalue_list) == 1){
          gen_stmt_assign2(node_get_child(lvalue_list, 0), id_temp);
        } else {
          gen_stmt_assign(lvalue_list, id_temp);
        }

        dowhile_semaphore++;
        gen_stmtlist_no_locals(node_get_child(stmt, 2));
        dowhile_semaphore--;
        sbuf_printf(sb_contents, "  }\n");
        pop_locals();
      }
      break;
    case STMT_SWITCH:
      {
        static int switch_counter = 0;
        const char* c_expr_value = mem_asprintf("_switch_expr%d", switch_counter);
        Node* expr = node_get_child(stmt, 0);
        Node* case_list = node_get_child(stmt, 1);

        sbuf_printf(sb_contents, "  {\n");
        sbuf_printf(sb_contents, "  Value %s = %s;\n", c_expr_value,
                                                       eval_expr(expr));
        int num_cases = node_num_children(case_list);
        for (int i = 0; i < num_cases; i++){
          Node* node_case = node_get_child(case_list, i);
          Node* node_case_expr = node_get_child(node_case, 0);
          Node* block = node_get_child(node_case, 1);
          const char* word = "else if";
          if (i == 0) word = "if";
          sbuf_printf(sb_contents, "  %s (op_equal(%s, %s) == VALUE_TRUE) {\n",
                      word, c_expr_value, eval_expr(node_case_expr));
          gen_stmtlist(block);
          sbuf_printf(sb_contents, "  }");
        }
        sbuf_printf(sb_contents, "  }\n");
      }
      break;
    case STMT_PASS:
      break;
    case STMT_RAISE:
      sbuf_printf(sb_contents, "  exc_raise(val_to_string(%s));\n",
                  eval_expr(node_get_child(stmt, 0)));
      break;
    default:
      err_node(stmt, "invalid statement type %d", stmt->type);
  }
}

static void gen_stmtlist(Node* stmtlist)
{
  push_locals();
  gen_stmtlist_no_locals(stmtlist);
  pop_locals();
}

static void gen_try(Node* try_block, Node* block_list, int i)
{
  Node* block = node_get_child(block_list, i);
  switch (block->type){
    case STMT_CATCH_ALL:
      sbuf_printf(sb_contents, "  EXC_CA_TRY\n");
      if (i == 0) {
        gen_stmtlist(try_block);
      } else {
        gen_try(try_block, block_list, i-1);
      }
      sbuf_printf(sb_contents, "  EXC_CA_CATCH\n");
      gen_stmtlist(block);
      sbuf_printf(sb_contents, "  EXC_CA_END\n");
      break;
    case STMT_FINALLY:
      {
        static int counter = 0;
        counter++;
        const char* lbl = mem_asprintf("_lbl_finally%d", counter);
        sbuf_printf(sb_contents, "  EXC_FIN_TRY(%s)\n", lbl);
        if (i == 0) {
          gen_stmtlist(try_block);
        } else {
          gen_try(try_block, block_list, i-1);
        }
        sbuf_printf(sb_contents, "  EXC_FIN_FINALLY(%s)\n", lbl);
        gen_stmtlist(block);
        sbuf_printf(sb_contents, "  EXC_FIN_END(%s)\n", lbl);
      }
      break;
  }
}

static void gen_stmtlist_no_locals(Node* stmtlist)
{
  int prev_stmt_type = 0;
  uint i = 0;
  while (i < stmtlist->children.size){
    Node* stmt = node_get_child(stmtlist, i);

    // Compile try-catch-finally
    if (stmt->type == STMT_TRY){
      Node* sublist = node_new(STMT_LIST);
      i++;
      while (i < stmtlist->children.size){
        Node* tmp = node_get_child(stmtlist, i);
        if (tmp->type == STMT_CATCH_ALL or
            tmp->type == STMT_FINALLY){
          node_add_child(sublist, tmp);
        } else break;
        i++;
      }

      int num_tblocks = node_num_children(sublist);
      if (num_tblocks == 0){
        err_node(stmt, "try block not followed by any other block");
      }
      gen_try(stmt, sublist, num_tblocks - 1);
      continue;
    }

    if (stmt->type == STMT_ELSE or stmt->type == STMT_ELIF){
      if (prev_stmt_type != STMT_IF and
          prev_stmt_type != STMT_ELIF){
        err_node(stmt, "statement must follow if or elif");
      }
    }
    prev_stmt_type = stmt->type;
    gen_stmt(stmt);
    i++;
  }
}

// Ensures that param_list is valid and returns if the function is vararg.
static bool check_vararg(Node* param_list)
{
  uint num_params = param_list->children.size;
  for (uint i = 0; i < num_params; i++){
    Node* param = node_get_child(param_list, i);
    if (node_has_string(param, "array")) {
      if (i != num_params - 1){
        err_node(param_list, "array_argument must be last");
      }
      return true;
    }
  }
  return false;
}

// Constructs the text of the form
//   Value __param1, Value __param2, Value __param3
// and registers locals while doing that.
static const char* gen_params(Node* param_list)
{
  StringBuf sb_params;
  sbuf_init(&sb_params, "");
  for (uint i = 0; i < param_list->children.size; i++){
    Node* param = node_get_child(param_list, i);

    const char* type = NULL;
    if (node_has_string(param, "array")) type = "Tuple";
    if (node_num_children(param) > 0){
      type = eval_type(node_get_child(param, 0));
    }
    const char* var_name = node_get_string(param, "name");
    const char* c_var_name = register_local(var_name, type);
    if (i == 0){
      sbuf_printf(&sb_params, "Value %s", c_var_name);
    } else {
      sbuf_printf(&sb_params, ", Value %s", c_var_name);
    }
  }
  const char* s = mem_strdup(sb_params.str);
  sbuf_deinit(&sb_params);
  return s;
}

// Generate all the statements, and optional return VALUE_NIL at the end.
static void gen_func_code(Node* stmt_list)
{
  dowhile_semaphore = 0;
  gen_stmtlist(stmt_list);
  // If last statement is of type STMT_RETURN, no need for another return.
  if (stmt_list->children.size == 0
        or
      node_get_child(stmt_list, stmt_list->children.size-1)->type != STMT_RETURN)
    sbuf_printf(sb_contents, "  return VALUE_NIL;\n");
}

static void gen_function(Node* function)
{
  // Deal with counter
  static uint counter = 0;
  counter++;
  const char* name = mem_asprintf("%s%s", module_get_prefix(),
                                  node_get_string(function, "name"));
  Node* rv_type = node_get_child(function, 0);
  Node* param_list = node_get_child(function, 1);
  Node* stmt_list = node_get_child(function, 2);
  uint num_params = param_list->children.size;
  context2 = CONTEXT_FUNC;

  // Generate
  push_locals();
  const char* c_name = mem_asprintf("_func%u_%s",
                                    counter,
                                    util_escape(name));
  sbuf_printf(sb_contents, "static Value %s(%s){\n",
              c_name, gen_params(param_list));
  sbuf_printf(sb_init1, "  Value v_%s = func%u_to_val(%s);\n",
              c_name, num_params, c_name);
  if (check_vararg(param_list))
    sbuf_printf(sb_init1, "  func_set_vararg(v_%s);\n", c_name);
  sbuf_printf(sb_init1, "  ssym_set(\"%s\", v_%s);\n", name, c_name);
  gen_func_code(stmt_list);
  pop_locals();

  sbuf_printf(sb_contents, "}\n");
}

static void gen_constructor(Node* constructor)
{
  context2 = CONTEXT_CONSTRUCTOR;

  // Get basic stuff.
  static int counter = 0;
  counter++;
  const char* r_constructor_name = mem_asprintf("%s.%s",
             context_class_name, node_get_string(constructor, "name"));
  const char* c_constructor_name = mem_asprintf("_cons%d_%s", counter,
                                        util_escape(r_constructor_name));

  Node* rv_type = node_get_child(constructor, 0);
  Node* param_list = node_get_child(constructor, 1);
  Node* stmt_list = node_get_child(constructor, 2);
  int num_params = param_list->children.size;

  push_locals();

  const char* c_value = mem_asprintf("v_constructor%d", counter);
  sbuf_printf(sb_init1, "  Value %s = func%d_to_val(%s);\n",
              c_value, num_params, c_constructor_name);
  if (check_vararg(param_list))
    sbuf_printf(sb_init1, "  func_set_vararg(%s);\n", c_value);
  sbuf_printf(sb_init1, "  ssym_set(\"%s\", %s);\n",
              r_constructor_name, c_value);

  sbuf_printf(sb_contents, "static Value %s(%s){\n",
              c_constructor_name, gen_params(param_list));
  set_local("self", "__self", context_class_name);
  sbuf_printf(sb_contents, "  %s* _c_data;\n", context_class_typedef);
  sbuf_printf(sb_contents, "  Value __self = obj_new(%s, (void**) &_c_data);\n",
              context_class_c_name);
  dowhile_semaphore = 0;
  gen_stmtlist(stmt_list);
  sbuf_printf(sb_contents, "  return __self;\n");
  pop_locals();
  sbuf_printf(sb_contents, "}\n");
}

static void gen_method(const char* method_name, Node* rv_type,
                       Node* param_list, Node* stmt_list)
{
  context2 = CONTEXT_METHOD;

  static int counter = 0;
  counter++;
  const char* r_method_name = mem_asprintf("%s.%s",
                                           context_class_name, method_name);
  const char* c_method_name = mem_asprintf("_met%d_%s", counter,
                                util_escape(r_method_name));

  // Prepend a node for "self"
  Node* node_self = node_new(PARAM);
  node_add_child(node_self, node_new_type(context_class_name));
  node_set_string(node_self, "name", "self");
  node_prepend_child(param_list, node_self);
  int num_params = param_list->children.size;

  push_locals();
  context2_method_value_name = mem_asprintf("v_method%d", counter);
  sbuf_printf(sb_init1, "  Value %s = func%d_to_val(%s);\n",
              context2_method_value_name, num_params, c_method_name);
  if (check_vararg(param_list))
    sbuf_printf(sb_init1, "  func_set_vararg(%s);\n", context2_method_value_name);
  sbuf_printf(sb_init1, "  ssym_set(\"%s\", %s);\n",
              r_method_name, context2_method_value_name);
  sbuf_printf(sb_init1, "  klass_new_method(%s, dsym_get(\"%s\"), "
                           "%s);\n", context_class_c_name,
                           method_name,
                           context2_method_value_name);

  sbuf_printf(sb_contents, "static Value %s(%s){\n",
              c_method_name, gen_params(param_list));
  if (context_class_type == CLASS_CDATA_OBJECT
      or context_class_type == CLASS_FIELD_OBJECT){
    sbuf_printf(sb_contents, "  %s* _c_data;\n", context_class_typedef);
    sbuf_printf(sb_contents, "  _c_data = (%s*) obj_c_data(__self);\n",
                context_class_typedef);
  }
  gen_func_code(stmt_list);
  pop_locals();

  sbuf_printf(sb_contents, "}\n");
}

static void gen_class(Node* klass)
{
  push_locals();
  static int counter = 0;
  counter++;
  context = CONTEXT_CLASS;
  context_class_name = mem_asprintf("%s%s",
                                    module_get_prefix(),
                                    node_get_string(klass, "name"));
  context_class_c_name = mem_asprintf("klass%d", counter);
  context_class_dict = dict_new(sizeof(char*), sizeof(char*),
                                dict_hash_string, dict_equal_string);
  sbuf_printf(sb_header, "static Klass* %s;\n", context_class_c_name);
  Node* ast = node_get_child(klass, 0);

  // Figure out the type of the object
  context_class_type = CLASS_VIRTUAL_OBJECT;
  for (int i = 0; i < ast->children.size; i++){
    Node* n = node_get_child(ast, i);
    if (n->type == C_CODE){
      if (context_class_type == CLASS_FIELD_OBJECT){
        err_node(klass, "class %s with both cdata and fields",
                    context_class_name);
      }
      context_class_type = CLASS_CDATA_OBJECT;
    }
    if (n->type == TL_VAR){
      if (context_class_type == CLASS_CDATA_OBJECT){
        err_node(klass, "class %s with both cdata and fields",
                    context_class_name);
      }
      context_class_type = CLASS_FIELD_OBJECT;
    }
  }

  // Generate class and any of the cdata/fields
  switch(context_class_type){
    case CLASS_CDATA_OBJECT:
      context_class_typedef = mem_asprintf("KlassCData%d", counter);
      sbuf_printf(sb_header, "typedef struct {\n");
      for (int i = 0; i < ast->children.size; i++){
        Node* n = node_get_child(ast, i);
        if (n->type == C_CODE){
          sbuf_printf(sb_header, "%s", util_trim_ends(n->text));
        }
      }
      sbuf_printf(sb_header, "} %s;\n", context_class_typedef);
      // TODO: Class parents
      sbuf_printf(sb_init1, "  %s = klass_new(dsym_get(\"%s\"), "
                             "dsym_get(\"%s\"), KLASS_CDATA_OBJECT, sizeof(%s));\n",
                  context_class_c_name, context_class_name, "Object",
                  context_class_typedef);
      break;
    case CLASS_FIELD_OBJECT:
      // _c_data is of the type Value* for field objects
      context_class_typedef = "Value";
      sbuf_printf(sb_init1, "  %s = klass_new(dsym_get(\"%s\"), "
                             "dsym_get(\"%s\"), KLASS_FIELD_OBJECT, 0);\n",
                  context_class_c_name, context_class_name, "Object");

      for (int i = 0; i < ast->children.size; i++){
        Node* n = node_get_child(ast, i);
        if (n->type == TL_VAR){
          const char* annotation = node_get_string(n, "annotation");
          const char* var_type = "";
          if (strcmp(annotation, "readable") == 0){
            var_type = "FIELD_READABLE";
          } else if (strcmp(annotation, "writable") == 0){
            var_type = "FIELD_READABLE | FIELD_WRITABLE";
          } else if (strcmp(annotation, "private") == 0){
            var_type = "0";
          } else {
            err_node(n, "invalid annotation '%s' inside class '%s'",
                            annotation, context_class_name);
          }

          Node* optassign_list = node_get_child(n, 0);
          for (int i = 0; i < node_num_children(optassign_list); i++){
            Node* optassign = node_get_child(optassign_list, i);
            const char* var_name = node_get_string(optassign, "name");
            sbuf_printf(sb_init1, "  klass_new_field(%s, dsym_get(\"%s\"), "
                                     "%s);\n", context_class_c_name, var_name,
                                     var_type);
          }
        }
      }
      break;
    case CLASS_VIRTUAL_OBJECT:
      context_class_typedef = NULL;
      sbuf_printf(sb_init1, "  %s = klass_new(dsym_get(\"%s\"), "
                             "dsym_get(\"%s\"), KLASS_VIRTUAL_OBJECT, 0);\n",
                  context_class_c_name, context_class_name, "Object");
  }

  // Generate all the methods and constructors
  for (int i = 0; i < ast->children.size; i++){
    Node* n = node_get_child(ast, i);
    switch(n->type){
      case FUNCTION:
        {
          Node* rv_type = node_get_child(n, 0);
          Node* param_list = node_get_child(n, 1);
          Node* stmt_list = node_get_child(n, 2);
          const char* name = node_get_string(n, "name");
          if (node_has_string(n, "annotation")){
            const char* annotation = node_get_string(n, "annotation");
            if (strcmp(annotation, "constructor")==0){
              gen_constructor(n);
            } else if (strcmp(annotation, "virtual_set")==0){
              gen_method(mem_asprintf("set_%s", name),
                         rv_type, param_list, stmt_list);
              sbuf_printf(sb_init1,
                          "  klass_new_virtual_writer(%s, dsym_get(\"%s\"), %s);\n",
                          context_class_c_name,
                          name,
                          context2_method_value_name);
            } else if (strcmp(annotation, "virtual_get")==0){
              gen_method(mem_asprintf("get_%s", name),
                         rv_type, param_list, stmt_list);
              sbuf_printf(sb_init1,
                          "  klass_new_virtual_reader(%s, dsym_get(\"%s\"), %s);\n",
                          context_class_c_name,
                          name,
                          context2_method_value_name);
            } else {
              err_node(n, "function annotated with '%s'"
                          " not allowed inside a class", annotation);
            }
          } else {
            gen_method(name, rv_type, param_list, stmt_list);
          }
        }
        break;
      case C_CODE:
      case TL_VAR:
        /* Ignore */
        break;
      default:
        err_node(n, "top level not allowed inside a class");
    }
  }
  context = CONTEXT_NONE;
  pop_locals();
}

// Iterate through the abstract syntax tree, and add all the global variables.
static void gen_globals(Node* ast)
{
  for (int i = 0; i < ast->children.size; i++){
    Node* n = node_get_child(ast, i);

    switch(n->type){
      case GLOBAL_VAR:
        {
          Node* optassign_list = node_get_child(n, 0);
          for (int i = 0; i < node_num_children(optassign_list); i++){
            Node* optassign = node_get_child(optassign_list, i);
            const char* var_name = node_get_string(optassign, "name");

            static int counter = 0;
            counter++;
            const char* ripe_name = mem_asprintf("%s%s", module_get_prefix(), var_name);
            const char* c_name = mem_asprintf("_glb%d_%s", counter,
                                              util_escape(ripe_name));

            set_local(ripe_name, c_name, NULL);
            sbuf_printf(sb_header, "static Value %s;\n", c_name);

            Node* right = node_get_child(optassign, 0);
            sbuf_printf(sb_init2, "  %s = %s;\n", c_name, eval_expr(right));
          }
        }
        break;
      case CONST_VAR:
        {
          Node* optassign_list = node_get_child(n, 0);
          for (int i = 0; i < node_num_children(optassign_list); i++){
            Node* optassign = node_get_child(optassign_list, i);
            const char* var_name = node_get_string(optassign, "name");

            static int counter = 0;
            counter++;
            const char* ripe_name = mem_asprintf("%s%s", module_get_prefix(), var_name);
            Node* right = node_get_child(optassign, 0);

            sbuf_printf(sb_init1, "  ssym_set(\"%s\", %s);\n",
                        ripe_name,
                        eval_expr(right));
          }
        }
        break;
      case MODULE:
        {
          const char* name = node_get_string(n, "name");
          module_push(name);
          Node* toplevels = node_get_child(n, 0);
          gen_globals(toplevels);
          module_pop(name);
        }
    }
  }
}

static void gen_toplevels(Node* ast)
{
  for (int i = 0; i < ast->children.size; i++){
    Node* n = node_get_child(ast, i);
    switch(n->type){
      case FUNCTION:
        gen_function(n);
        break;
      case MODULE:
        {
          const char* name = node_get_string(n, "name");
          module_push(name);
          Node* toplevels = node_get_child(n, 0);
          gen_toplevels(toplevels);
          module_pop(name);
        }
        break;
      case C_CODE:
        {
          sbuf_printf(sb_header, "%s\n", util_trim_ends(n->text));
        }
        break;
      case GLOBAL_VAR:
      case TL_VAR:
      case CONST_VAR:
        // Ignore.
        break;
      case CLASS:
        {
          gen_class(n);
        }
        break;
      default:
        assert_never();
    }
  }
}

int generate(Node* ast, const char* module_name, const char* i_source_filename)
{
  tables_init();
  context = CONTEXT_NONE;

  // Set up error handling.
  err_filename = i_source_filename;

  // First item on the locals stack refers to the global variables.
  push_locals();
  // Globals have to be done separately of the rest, because they don't depend
  // on location in the file.
  gen_globals(ast);
  gen_toplevels(ast);
  pop_locals();

  return 0;
}
