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
#include "lang/lang.h"

//////////////////////////////////////////////////////////////////////////////
// ripe/cli.c
//////////////////////////////////////////////////////////////////////////////

const char* path_join(int num, ...);
const char* path_get_extension(const char* path);

typedef struct {
  Array keys;
  Array values;
} Conf;
void conf_load(Conf* conf, const char* filename);
const char* conf_query(Conf* conf, const char* key);

//////////////////////////////////////////////////////////////////////////////
// ripe/ast.c
//////////////////////////////////////////////////////////////////////////////

const char* namespace_get_prefix();
void namespace_pop();
void namespace_push(const char* name);

//////////////////////////////////////////////////////////////////////////////
// ripe/error.c
//////////////////////////////////////////////////////////////////////////////

extern const char* err_filename;
extern int log_verbosity;
void err_node(Node* node, const char* format, ...);
void err(const char* format, ...);

//////////////////////////////////////////////////////////////////////////////
// ripe/dump.c
//////////////////////////////////////////////////////////////////////////////

extern StringBuf* sb_contents;
extern StringBuf* sb_header;
extern StringBuf* sb_init1a;
extern StringBuf* sb_init1b;
extern StringBuf* sb_init2;
extern StringBuf* sb_init3;

void dump_init();
void dump_output(FILE* f, const char* module_name);

//////////////////////////////////////////////////////////////////////////////
// ripe/util.c
//////////////////////////////////////////////////////////////////////////////

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
