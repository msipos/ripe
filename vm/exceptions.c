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

#define EXC_STACK_SIZE 2048

char* exc_stack[EXC_STACK_SIZE * sizeof(jmp_buf)];
static int stack_idx;

#define copy_jmp_buf(d,s) memcpy(d, s, sizeof(jmp_buf))

void exc_init()
{
  stack_idx = 0;
}

void exc_raise(char* format, ...)
{
  if (stack_idx == 0){
    va_list ap;
    va_start (ap, format);
    fprintf(stderr, "uncaught Exception: ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end (ap);
    exit(1);
  } else {
    jmp_buf jb;
    stack_idx--;
    void* src = exc_stack + sizeof(jmp_buf) * stack_idx;
    copy_jmp_buf(jb, src);
    longjmp(jb, 1);
  }
}

void exc_register_any2()
{
  void* dest = exc_stack + sizeof(jmp_buf) * stack_idx;
  copy_jmp_buf(dest, exc_jb);
  stack_idx++;
}

