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

#ifndef LANG_H
#define LANG_H

#include "clib/clib.h"

//////////////////////////////////////////////////////////////////////////////
// lang/astnode.c
//////////////////////////////////////////////////////////////////////////////

typedef struct {
  int type;              // Node type.
  Array children;        // Array of Node*.
  Dict props_strings;    // String properties.
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
// lang/input.c
//////////////////////////////////////////////////////////////////////////////
typedef struct {
  const char* filename;
  Array lines;
} RipeInput;
int input_from_file(RipeInput* input, const char* filename);

//////////////////////////////////////////////////////////////////////////////
// lang/pp.c
//////////////////////////////////////////////////////////////////////////////

int preprocess(RipeInput* input);

//////////////////////////////////////////////////////////////////////////////
// ripe/build-tree.c
//////////////////////////////////////////////////////////////////////////////

// Error in case build_tree returns NULL (no new line).
extern const char* build_tree_error;

// Parse the given file.
Node* build_tree(RipeInput* in);

// Interface to bison & flex
#define YYSTYPE Node*
extern Node* rc_result;
void rc_error(const char*s);
int rc_parse();
int rc_lex();
int input_read(char* buf, int max_size); // Used by flex to do reading

// Types of AST nonterminal nodes:
#define TOPLEVEL_LIST     1000
#define NAMESPACE         1001
#define FUNCTION          1002
#define STMT_LIST         1003
#define CLASS             1004
#define PARAM             1006
#define TL_VAR            1008  // Top-level variable (with annotation maybe)
                                // Going to be deprecated
#define GLOBAL_VAR        1007
#define CONST_VAR         1009

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
#define STMT_SWITCH       1115

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
#define CASE_LIST         1056
#define CASE              1057

#include "lang/parser.h"
#include "lang/scanner.h"

#endif
