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

// Prototype for module initializers
void ripe_module1();
void ripe_module2();
void ripe_module3();

int main(int argc, char** argv)
{
  // Provide argc and argv to interested modules
  sys_argc = argc;
  sys_argv = argv;

  // Initialize memory system
  mem_init();

  // Initialize stack and exception system
  stack_init();

  // Initialize static symbol table
  sym_init();

  // Initialize value klass system
  klass_init();

  // Phase 1
  stack_push_annotation("init1_Function");
    init1_Function();
  stack_pop();
  stack_push_annotation("init1_Object");
    init1_Object();
  stack_pop();
  stack_push_annotation("init1_Complex");
    init1_Complex();
  stack_pop();

  ripe_module1();

  // Phase 1.5
  stack_push_annotation("phase 1.5");
    common_init_phase15();
    klass_init_phase15();
  stack_pop();

  // Phase 2
  stack_push_annotation("init2_Function");
    init2_Function();
  stack_pop();
  stack_push_annotation("init2_Object");
    init2_Object();
  stack_pop();
  stack_push_annotation("init2_Complex");
    init2_Complex();
  stack_pop();

  ripe_module2();
  ripe_module3();

  // Lookup main symbol
  Value sym_main = ssym_get("main");

  // Call main.
  Value rv = func_call0(sym_main);
  if (is_int64(rv)){
    return unpack_int64(rv);
  }
  mem_deinit();
  return 0;
}
