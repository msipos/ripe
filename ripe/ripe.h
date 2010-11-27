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

#ifndef RIPE_H
#define RIPE_H

#include "clib/clib.h"
#include "clib/stringbuf.h"

//////////////////////////////////////////////////////////////////////////////
// ripe/cli.c
//////////////////////////////////////////////////////////////////////////////

const char* path_join(int num, ...);
const char* path_get_app_dir();
const char* path_get_extension(const char* path);
bool path_exists(const char* filename);

typedef struct {
  Array keys;
  Array values;
} Conf;
void conf_load(Conf* conf, const char* filename);
const char* conf_query(Conf* conf, const char* key);

//////////////////////////////////////////////////////////////////////////////
// ripe/astnode.c
//////////////////////////////////////////////////////////////////////////////

typedef struct {
  int type;              // Node type.
  Array children;        // Array of Node*.
  Dict props_strings;     // String properties.
  Dict props_nodes;      // Node properties.
  // For token nodes:
  const char* text;      // Text of the token.
  const char* filename;  // Filename.
  int line;              // Line number
} Node;

// Create a token node. Does not duplicate text or filename strings.
Node* node_new_token(int type, const char* text, const char* filename, int line);
Node* node_new(int type);
Node* node_new_inherit(int type, Node* ancestor);

// Children
void  node_add_child(Node* parent, Node* child);
void node_prepend_child(Node* parent, Node* child);
uint  node_num_children(Node* parent);
Node* node_get_child(Node* parent, uint i);
void node_extend_children(Node* new_parent, Node* old_parent);

// Properties
void node_set_string(Node* n, const char* key, const char* value);
char* node_get_string(Node* n, const char* key);
bool node_has_string(Node* n, const char* key);

void node_draw(Node* ast);

Node* node_new_id(const char* id);
Node* node_new_int(int64 i);
Node* node_new_expr_index1(Node* left, Node* index);
Node* node_new_expr_list();
Node* node_new_field_call(Node* callee, char* field_name, int64 num, ...);
Node* node_new_type(const char* type);

//////////////////////////////////////////////////////////////////////////////
// ripe/ast.c
//////////////////////////////////////////////////////////////////////////////

const char* module_get_prefix();
void module_pop();
void module_push(const char* name);
const char* eval_type(Node* type_node);

//////////////////////////////////////////////////////////////////////////////
// ripe/error.c
//////////////////////////////////////////////////////////////////////////////

extern const char* err_filename;
extern int log_verbosity;
void err_node(Node* node, const char* format, ...);
void err(const char* format, ...);
void warn(const char* format, ...);
void logging(const char* format, ...);

//////////////////////////////////////////////////////////////////////////////
// ripe/dump.c
//////////////////////////////////////////////////////////////////////////////

extern StringBuf* sb_contents;
extern StringBuf* sb_header;
extern StringBuf* sb_init1;
extern StringBuf* sb_init2;

void dump_init();
void dump_output(FILE* f, const char* module_name);

//////////////////////////////////////////////////////////////////////////////
// ripe/generator.c
//////////////////////////////////////////////////////////////////////////////

// Returns non-zero in case of an error.
int generate(Node* ast, const char* module_name, const char* source_filename);

// Dump type info
void generate_type_info(Node* ast);

//////////////////////////////////////////////////////////////////////////////
// ripe/operator.c
//////////////////////////////////////////////////////////////////////////////

bool is_unary_op(Node* node);
const char* unary_op_map(int type);

bool is_binary_op(Node* node);
const char* binary_op_map(int type);

//////////////////////////////////////////////////////////////////////////////
// ripe/typer.c
//////////////////////////////////////////////////////////////////////////////

typedef struct {
  const char* name;
  // For a function or method, this is the return type:
  const char* rv;
  int num_params;
  const char** param_types;
} TyperRecord;

void typer_init();
void typer_add(TyperRecord* tr);
TyperRecord* typer_query(const char* name);
void typer_ast(Node* ast);
void typer_dump(FILE* f);
void typer_load(FILE* f);
const char* typer_infer(Node* expr);
bool typer_needs_check(const char* destination, const char* source);

//////////////////////////////////////////////////////////////////////////////
// ripe/build-tree.c
//////////////////////////////////////////////////////////////////////////////

// Parse the given file.
Node* build_tree(const char* filename);

// Dump the tokens read from a file.
void dump_tokens(const char* filename);

// Interface to bison
#define YYSTYPE Node*
extern Node* rc_result;
void rc_error(const char*s);
int rc_parse();
int rc_lex();

// Types of AST nonterminal nodes:
#define TOPLEVEL_LIST     1000
#define MODULE            1001
#define FUNCTION          1002
#define STMT_LIST         1003
#define CLASS             1004
#define PARAM             1006
#define TL_VAR            1008  // Top-level variable (with annotation maybe)

// Types of STMTs
#define STMT_EXPR         1100
#define STMT_RETURN       1101
#define STMT_IF           1102
#define STMT_ELIF         1103
#define STMT_ELSE         1104
#define STMT_PASS         1105
#define STMT_ASSIGN       1106

#define STMT_WHILE        1110
#define STMT_BREAK        1111
#define STMT_CONTINUE     1112
#define STMT_FOR          1113
#define STMT_LOOP         1114

#define STMT_TRY          1120
#define STMT_CATCH_ALL    1121
#define STMT_FINALLY      1122
#define STMT_RAISE        1123

// Types of EXPRs
#define EXPR_ID_CALL      1030
#define EXPR_FIELD_CALL   1031
#define EXPR_INDEX        1032
#define EXPR_ARRAY        1033
#define EXPR_FIELD        1034
#define EXPR_AT_VAR       1036
#define EXPR_RANGE_BOUNDED        1040
#define EXPR_RANGE_BOUNDED_LEFT   1041
#define EXPR_RANGE_BOUNDED_RIGHT  1042
#define EXPR_RANGE_UNBOUNDED      1043
#define EXPR_IS_TYPE      1044
#define EXPR_TYPED_ID     1045

// Helper nonterminal nodes
#define ID_LIST           1050
#define EXPR_LIST         1051
#define PARAM_LIST        1052
#define OPTASSIGN_LIST    1053
#define OPTASSIGN         1054
#define TYPE              1055

#include "ripe/parser.h"
#include "ripe/scanner.h"

//////////////////////////////////////////////////////////////////////////////
// ripe/util.c
//////////////////////////////////////////////////////////////////////////////

// Remove first and last character of input.
const char* util_trim_ends(const char* input);
// In str, replace each character c by string replace
const char* util_replace(const char* str, const char c, const char* replace);
// Replace all occurences of '?', '!' and '.' with '_'
const char* util_escape(const char* input);
// Escape and prepend "__".
const char* util_make_c_name(const char* ripe_name);

//////////////////////////////////////////////////////////////////////////////
// ripe/vars.c
//////////////////////////////////////////////////////////////////////////////

typedef struct {
  const char* c_name;
  const char* ripe_name;
  const char* type;
} Variable;

void push_locals();
void pop_locals();
// TODO: Clear up confusion between set_local and register_local
void set_local(const char* ripe_name, const char* c_name, const char* type);
const char* register_local(const char* ripe_name, const char* type);
Variable* query_local_full(const char* ripe_name);
const char* query_local(const char* ripe_name);

#endif
