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

int main(int argc, char** argv)
{
  // Provide argc and argv to interested modules
  sys_argc = argc;
  sys_argv = argv;

  #ifdef CLIB_GC
  GC_INIT();
  #endif

  // Initialize exception system
  exc_init();

  // Initialize static symbol table
  sym_init();

  // Initialize value klass system
  klass_init();

  // Phase 1
  init1_Function();
  init1_Object();
  init1_Integer();
  init1_Double();
  init1_Arrays();
  init1_Range();
  init1_Complex();
  init1_Map();
  ripe_module1();

  // Phase 1.5
  common_init_phase15();
  klass_init_phase15();

  // Phase 2
  init2_Function();
  init2_Object();
  init2_Integer();
  init2_Double();
  init2_Arrays();
  init2_Range();
  init2_Complex();
  init2_Map();
  ripe_module2();

  // Lookup main symbol
  Value sym_main = ssym_get("main");

  // Call main.
  func_call0(sym_main);
}
