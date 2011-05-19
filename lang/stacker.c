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

typedef struct {
  int type;
  const char* break_label;
  const char* continue_label;
} StackerElement;

SArray stacker;

void stacker_init()
{
  sarray_init(&stacker);
}

void stacker_push(int type, const char* break_label, const char* continue_label)
{
  StackerElement* el = mem_new(StackerElement);
  el->type = type;
  el->break_label = break_label;
  el->continue_label = continue_label;
  sarray_append_ptr(&stacker, el);
}

void stacker_pop()
{
  sarray_pop(&stacker);
}
