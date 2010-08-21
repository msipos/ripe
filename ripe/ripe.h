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

// Properties
void node_set_string(Node* n, const char* key, const char* value);
char* node_get_string(Node* n, const char* key);
bool node_has_string(Node* n, const char* key);

void node_draw(Node* ast);

//////////////////////////////////////////////////////////////////////////////
// ripe/generator.c
//////////////////////////////////////////////////////////////////////////////

// Returns non-zero in case of an error.
int generate(Node* ast, const char* module_name, const char* source_filename,
             const char* output_filename);

//////////////////////////////////////////////////////////////////////////////
// ripe/operator.c
//////////////////////////////////////////////////////////////////////////////

bool is_unary_op(Node* node);
const char* unary_op_map(int type);

bool is_binary_op(Node* node);
const char* binary_op_map(int type);

//////////////////////////////////////////////////////////////////////////////
// ripe/build-tree.c
//////////////////////////////////////////////////////////////////////////////

// Parse the given file. Returns NULL in case of error.
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
#define GLOBAL_VAR        1005
#define ARRAY_ARG         1006
#define ANNOT_FUNCTION    1007
#define TL_VAR            1008  // Top-level variable (with annotation maybe)
#define CONST             1009

// Types of STMTs
#define STMT_EXPR         1010
#define STMT_RETURN       1011
#define STMT_IF           1012
#define STMT_ELIF         1013
#define STMT_ELSE         1014
#define STMT_PASS         1015
#define STMT_ASSIGN       1016
#define STMT_TRY          1019

#define STMT_WHILE        1020
#define STMT_BREAK        1021
#define STMT_CONTINUE     1022
#define STMT_FOR          1023
#define STMT_LOOP         1024

// Types of EXPRs
#define EXPR_ID_CALL      1030
#define EXPR_FIELD_CALL   1031
#define EXPR_INDEX        1032
#define EXPR_ARRAY        1033
#define EXPR_FIELD        1034
#define EXPR_RANGE        1035
#define EXPR_AT_VAR       1036
// Helper nonterminal nodes
#define ID_LIST           1050
#define EXPR_LIST         1051
#define ARG_LIST          1052

#include "ripe/parser.h"
#include "ripe/scanner.h"

#endif
