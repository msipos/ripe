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

#define TYPE_FUNC       1
#define TYPE_CATCH      2
#define TYPE_CATCH_ALL  3
#define TYPE_FINALLY    4
#define TYPE_ANNOTATION 5
typedef struct {
  int type;
  Value func;
  Klass* exc_type;
  char jb[sizeof(jmp_buf)];
  char* annotation;
} Element;

#define STACK_SIZE 2000
THREAD_LOCAL Element stack[STACK_SIZE];
THREAD_LOCAL int64 stack_idx = 0;

void stack_init()
{
  stack_unwinding = false;
}

#define copy_jmp_buf(d, s)  memcpy(d, s, sizeof(jmp_buf))

void stack_push_catch_all()
{
  stack[stack_idx].type = TYPE_CATCH_ALL;
  copy_jmp_buf(stack[stack_idx].jb, exc_jb);
  stack_idx++;
}

void stack_check_unwinding()
{
  if (stack_unwinding){
    longjmp(exc_unwinding_jb, 1);
  }
}

void stack_push_catch(Klass* exc_type)
{
  stack[stack_idx].type = TYPE_CATCH;
  stack[stack_idx].exc_type = exc_type;
  copy_jmp_buf(stack[stack_idx].jb, exc_jb);
  stack_idx++;
}

void stack_push_finally()
{
  stack[stack_idx].type = TYPE_FINALLY;
  copy_jmp_buf(stack[stack_idx].jb, exc_jb);
  stack_idx++;
}

void stack_push_func(Value func)
{
  stack[stack_idx].type = TYPE_FUNC;
  stack[stack_idx].func = func;
  stack_idx++;
}

void stack_push_annotation(char* annotation)
{
  slog("stack_push_annotation(): '%s' idx = %"PRId64, annotation, stack_idx);
  stack[stack_idx].type = TYPE_ANNOTATION;
  stack[stack_idx].annotation = annotation;
  stack_idx++;
}

void stack_pop()
{
  slog("stack_pop(): idx = %"PRId64, stack_idx);
  assert(stack_idx > 0);
  stack_idx--;
}

void stack_display()
{
  for(int64 i = 0; i < stack_idx; i++){
    const char* name = "invalid";
    switch(stack[i].type){
      case TYPE_FUNC:
        name = ssym_reverse_get(stack[i].func);
        if (name == NULL) name = "?";
        break;
      case TYPE_CATCH_ALL:
        name = "try-catch-all";
        break;
      case TYPE_FINALLY:
        name = "try-finally";
        break;
      case TYPE_ANNOTATION:
        name = stack[i].annotation;
        break;
    }

    if (i == stack_idx - 1){
      fprintf(stderr, "in %s():\n", name);
    } else {
      fprintf(stderr, "from %s():\n", name);
    }
  }
}

void exc_raise(char* format, ...)
{
  int64 backup_stack_idx = stack_idx;

  // TODO: Go up the stack and try to match an exception.
  stack_unwinding = true;
  for(stack_idx--; stack_idx >= 0; stack_idx--){
    switch(stack[stack_idx].type){
      case TYPE_CATCH_ALL:
        copy_jmp_buf(exc_jb, stack[stack_idx].jb);
        stack_unwinding = false;
        longjmp(exc_jb, 1);
        break;
      case TYPE_FINALLY:
        if (setjmp(exc_unwinding_jb) == 0) {
          copy_jmp_buf(exc_jb, stack[stack_idx].jb);
          longjmp(exc_jb, 1);
        }
        break;
    }
  }
  stack_unwinding = false;

  stack_idx = backup_stack_idx;

  // If you're here, display the stack and terminate
  stack_display();

  va_list ap;
  va_start (ap, format);
  fprintf(stderr, "  uncaught Exception: ");
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  va_end (ap);
  exit(1);
}
