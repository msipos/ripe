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
#include "lang/lang.h"

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
