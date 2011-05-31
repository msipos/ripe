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

const char* stacker_label()
{
  static int counter = 0;
  counter++;
  return mem_asprintf("_lbl_%d", counter);
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

int stacker_size()
{
  return stacker.size;
}

static StackerElement* stacker_unwind(int num)
{
  if (num < 1){
    fatal_throw("invalid number of loops to unwind: %d", num);
  }

  int cur = stacker.size;
  for(;;){
    cur--;
    num--;
    if (cur < 0) {
      fatal_throw("break or continue but not enough loops");
    }
    
    StackerElement* el = sarray_get_ptr(&stacker, cur);
    if (el->type == STACKER_TRY or el->type == STACKER_CATCH
        or el->type == STACKER_FINALLY) {
      fatal_throw("break or continue crosses try/catch/finally boundaries");
    }
    
    if (num == 0) return el;
  }
}

const char* stacker_break(int num)
{
  fatal_push("while unwinding 'break %d'", num);
  const char* rv = stacker_unwind(num)->break_label;
  fatal_pop();
  return rv;
}

const char* stacker_continue(int num)
{
  fatal_push("while unwinding 'continue %d'", num);
  const char* rv = stacker_unwind(num)->continue_label;
  fatal_pop();
  return rv;
}

