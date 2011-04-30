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

#endif
