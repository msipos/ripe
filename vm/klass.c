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

Array klasses;
Dict dsym_to_klass;

void klass_init()
{
  array_init(&klasses, Klass*);
  // Initialize dsym_to_klass
  dict_init(&dsym_to_klass, sizeof(Value), sizeof(Klass*), dict_hash_uint32,
            dict_equal_uint32);
}

void klass_dump()
{
  for (int i = 0; i < klasses.size; i++){
    Klass* klass = array_get(&klasses, Klass*, i);
    printf("%p %s %d\n", klass, dsym_reverse_get(klass->name), klass->obj_size);
  }
}

// Create Klass structure and add it to the dictionary.
Klass* klass_new(Value name, int cdata_size)
{
  if (dict_query(&dsym_to_klass, &name, NULL)){
    fprintf(stderr, "error: class '%s' initialized twice\n",
            dsym_reverse_get(name));
    exit(1);
  }

  Klass* klass = mem_new(Klass);
  klass->name = name;
  klass->cdata_size = cdata_size;
  // This preliminary calculation of obj_size is necessary because of Function
  // klass.
  klass->obj_size = sizeof(Klass*) + cdata_size;
  dict_init(&(klass->methods), sizeof(Value), sizeof(Value), dict_hash_uint32,
            dict_equal_uint32);
  dict_init(&(klass->readable_fields), sizeof(Value), sizeof(uint64),
            dict_hash_uint32, dict_equal_uint32);
  dict_init(&(klass->writable_fields), sizeof(Value), sizeof(uint64),
            dict_hash_uint32, dict_equal_uint32);
  dict_init(&(klass->fields), sizeof(Value), sizeof(uint64),
            dict_hash_uint32, dict_equal_uint32);
  klass->num_fields = 0;

  array_append(&klasses, klass);
  dict_set(&dsym_to_klass, &name, &klass);

  return klass;
}

int klass_new_field(Klass* klass, Value name, int type)
{
  int64 field_num = klass->num_fields;
  klass->num_fields++;
  dict_set(&(klass->fields), &name, &field_num);
  if (type & FIELD_READABLE){
    dict_set(&(klass->readable_fields), &name, &field_num);
  }
  if (type & FIELD_WRITABLE){
    dict_set(&(klass->writable_fields), &name, &field_num);
  }
  return field_num;
}

int klass_get_field_int(Klass* klass, Value name)
{
  int64 field_num;
  if (dict_query(&(klass->fields), &name, &field_num)){
    return field_num;
  } else {
    fprintf(stderr, "class '%s' does not have field '%s'\n",
            dsym_reverse_get(klass->name),
            dsym_reverse_get(name));
    exit(1);
  }
}

void klass_new_virtual_reader(Klass* klass, Value name, Value func)
{
  dict_set(&(klass->readable_fields), &name, &func);
}

void klass_new_virtual_writer(Klass* klass, Value name, Value func)
{
  dict_set(&(klass->writable_fields), &name, &func);
}

void klass_new_method(Klass* klass, Value name, Value method)
{
  dict_set(&(klass->methods), &name, &method);
}

void klass_init_phase15()
{
  // Finally, go thru list again, and finalize it:
  // put in all the methods, fields and cdata of the parent,
  // put in all your own methods, fields and cdata,
  // verify that only one cdata is defined by someone in the hierarchy.
  for (int i = 0; i < klasses.size; i++){
    Klass* klass = array_get(&klasses, Klass*, i);
    if (klass->cdata_size > 0 and klass->num_fields > 0){
      exc_raise("class '%s' has both cdata and fields", dsym_reverse_get(klass->name));
    }
    klass->obj_size = sizeof(Klass*) + klass->num_fields * sizeof(Value) + klass->cdata_size;
  }
}

Klass* klass_get(Value name)
{
  Klass* klass;
  if (dict_query(&dsym_to_klass, &name, &klass)){
    return klass;
  }
  exc_raise("no such class: %s", dsym_reverse_get(name));
}

Value obj_new(Klass* klass, void** data)
{
  void* obj = mem_malloc(klass->obj_size);
  *data = obj + sizeof(Klass*);
  *((Klass**) obj) = klass;
  return pack_ptr(obj);
}
Value obj_new2(Klass* klass)
{
  void* obj = mem_malloc(klass->obj_size);
  *((Klass**) obj) = klass;
  return pack_ptr(obj);
}

Value obj_verify_assign(Value v_obj, Klass* klass)
{
  Klass* klass_obj = obj_klass(v_obj);
  if (klass_obj != klass){
    exc_raise("TypeError: tried to assign expression of type %s"
              " to a variable of type %s",
              dsym_reverse_get(klass_obj->name),
              dsym_reverse_get(klass->name));
  }
  return v_obj;
}

Value field_get(Value v_obj, Value field)
{
  Klass* klass = obj_klass(v_obj);
  uint64 field_num;
  if (dict_query(&(klass->readable_fields), &field, &field_num)){
    if (field_num < 1024){
      Value* c_data = obj_c_data(v_obj);
      return c_data[field_num];
    } else {
      return func_call1((Value) field_num, v_obj);
    }
  } else {
    exc_raise("object of class %s does not have a readable field '%s'",
              dsym_reverse_get(klass->name),
              dsym_reverse_get(field));
  }
}

void field_set(Value v_obj, Value field, Value val)
{
  Klass* klass = obj_klass(v_obj);
  uint64 field_num;
  if (dict_query(&(klass->writable_fields), &field, &field_num)){
    if (field_num < 1024){
      Value* c_data = obj_c_data(v_obj);
      c_data[field_num] = val;
    } else {
      func_call2((Value) field_num, v_obj, val);
    }
  } else {
    exc_raise("object of class %s does not have a writable field '%s'",
              dsym_reverse_get(klass->name),
              dsym_reverse_get(field));
  }
}

void method_error(Klass* klass, Value dsym)
{
  exc_raise("class '%s' does not have method '%s'",
            dsym_reverse_get(klass->name), dsym_reverse_get(dsym));
}
