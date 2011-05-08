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

Klass* klass_func;

void init1_Function(){
  klass_func = klass_new(dsym_get("Function"),
                         sizeof(Func));
}

void init2_Function(){
}

void func_set_vararg(Value v_func)
{
  obj_verify(v_func, klass_func);
  Func* c_data = obj_c_data(v_func);
  c_data->var_params = 1;
}

void* func_get_ptr(Value v_func, int16 num_params)
{
  obj_verify(v_func, klass_func);
  Func* c_data = obj_c_data(v_func);
  if (c_data->num_params != num_params){
    exc_raise("func_get_ptr() mismatch (Function has %"PRIu16
              " but called with %"PRIu16")", c_data->num_params,
              num_params);
  }
  return (void*) c_data->func0;
}

Value func_to_val(void* c_func, int num_params)
{
  Func* func;
  Value f = obj_new(klass_func, (void**) &func);
  func->var_params = 0;
  func->num_params = num_params;
  func->func = c_func;
  func->is_block = false;
  func->block_elems = 0;
  func->block_data = NULL;
  return f;
}

Value block_to_val(void* c_func, int num_params, int block_elems, ...)
{
  Func* func;
  Value f = obj_new(klass_func, (void**) &func);
  func->var_params = 0;
  func->num_params = num_params;
  func->func = c_func;
  func->is_block = true;
  func->block_elems = block_elems;
  
  va_list ap;
  va_start(ap, block_elems);
  func->block_data = mem_malloc(block_elems * sizeof(Value));
  for (int i = 0; i < block_elems; i++) func->block_data[i] = va_arg(ap, Value);
  return f;
}
