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

#define TYPE_ANNOT       1
#define TYPE_CATCH_ALL   2
#define TYPE_CATCH       3
#define TYPE_FINALLY     4
//#define TYPE_ANNOTATION 5
typedef struct {
  int type;
  Klass* exc_type;
  char jb[sizeof(jmp_buf)];
  char* annotation;
} Element;

#define STACK_SIZE 2000
THREAD_LOCAL Element stack[STACK_SIZE];
THREAD_LOCAL int stack_idx = 0;

// TODO: Can these be made THREAD_LOCAL?
THREAD_LOCAL jmp_buf exc_jb;
THREAD_LOCAL jmp_buf exc_unwinding_jb;
THREAD_LOCAL bool stack_unwinding = false;
THREAD_LOCAL Value exc_obj;

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

void stack_pop()
{
  assert(stack_idx > 0);
  stack_idx--;
}

/*


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
*/

void stack_annot_push(char* annotation)
{
  stack[stack_idx].type = TYPE_ANNOT;
  stack[stack_idx].annotation = annotation;
  stack_idx++;
}

void stack_annot_pop()
{
  for(;;){
    assert(stack_idx > 0);
    stack_idx--;
    if (stack[stack_idx].type == TYPE_ANNOT) break;
  }
}

void stack_display()
{
  for(int64 i = 0; i < stack_idx; i++){
    const char* name = "invalid";
    switch(stack[i].type){
      case TYPE_ANNOT:
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
  va_list ap;
  va_start (ap, format);
  int sz = 1024 + strlen(format)*10;
  char buf[sz];
  vsnprintf(buf, sz, format, ap);
  va_end (ap);

  Value obj = obj_new2(klass_Error);
  field_set(obj, dsym_text, string_to_val(buf));
  exc_raise_object(obj);
}

void exc_raise_object(Value obj)
{
  int backup_stack_idx = stack_idx;
  stack_unwinding = true;
  for(;;){
    if (stack_idx == 0) break;
    stack_idx--;
    
    switch(stack[stack_idx].type){
      case TYPE_ANNOT:
        // Ignore.
        break;
      case TYPE_CATCH_ALL:
        // Restore to this handler.
        stack_unwinding = false;
        copy_jmp_buf(exc_jb, stack[stack_idx].jb);
        longjmp(exc_jb, 1);
        break;
      case TYPE_CATCH:
        if (obj_eq_klass(obj, stack[stack_idx].exc_type)){
          stack_unwinding = false;
          exc_obj = obj;
          copy_jmp_buf(exc_jb, stack[stack_idx].jb);
          longjmp(exc_jb, 1);
        }
        break;
      case TYPE_FINALLY:
        if (setjmp(exc_unwinding_jb) == 0){
          copy_jmp_buf(exc_jb, stack[stack_idx].jb);
          longjmp(exc_jb, 1);
        }
        break;
    }
  }

  // If you're here, display the stack and terminate
  stack_idx = backup_stack_idx; 
  stack_display();
  fprintf(stderr, "  uncaught exception of type %s", 
          dsym_reverse_get(
            obj_klass(obj)->name
          )
         );

  bool do_to_string = false;
  if (field_has(obj, dsym_text)){
    Value vtext = field_get(obj, dsym_text);
    if (obj_klass(vtext) == klass_String){
      fprintf(stderr, ": '%s'\n", val_to_string(vtext));   
    } else {
      fprintf(stderr, "with unreadable text field\n");
      do_to_string = true;
    }
  } else {
    fprintf(stderr, " with no text field\n");
    do_to_string = true;
  }
  
  if (do_to_string){
    if (method_has(obj, dsym_to_string)){
      Value vtext = method_call0(obj, dsym_to_string);
      if (obj_klass(vtext) == klass_String){
        fprintf(stderr, "  object representation: '%s'\n", val_to_string(vtext));
      }
    }
  }
  
  exit(1);
}

