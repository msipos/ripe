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

#include "vm/vm.h"
#include <setjmp.h>
#include <stdarg.h>

#define TYPE_STACK 1

typedef struct {
  int type;
  Value func;
} Element;

#define STACK_SIZE 1000
Element stack[STACK_SIZE];
int64 stack_idx = 0;

void stack_init()
{

}

void stack_push(Value func)
{
  stack[stack_idx].type = TYPE_STACK;
  stack[stack_idx].func = func;
  stack_idx++;
}

void stack_pop()
{
  assert(stack_idx > 0);
  stack_idx--;
}

void exc_raise(char* format, ...)
{
  // TODO: Go up the stack and try to match an exception.

  for(int64 i = 0; i < stack_idx; i++){
    const char* name;
    name = ssym_reverse_get(stack[i].func);
    if (name == NULL) name = "?";

    if (i == stack_idx - 1){
      fprintf(stderr, "in %s():\n", name);
    } else {
      fprintf(stderr, "from %s():\n", name);
    }
  }

  va_list ap;
  va_start (ap, format);
  fprintf(stderr, "  uncaught Exception: ");
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  va_end (ap);
  exit(1);
}

void exc_register_any2()
{
}