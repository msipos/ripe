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

#ifndef VM_H
#define VM_H

#include "clib/clib.h"

//////////////////////////////////////////////////////////////////////////////
// Value
//////////////////////////////////////////////////////////////////////////////
typedef uint64 Value;
#include "vm/value_inline.c"

//////////////////////////////////////////////////////////////////////////////
// sym-table.c
//////////////////////////////////////////////////////////////////////////////
typedef uint32 Dsym;
void sym_init();
Value ssym_get(const char* name);
Value ssym_set(const char* name, Value val);
Dsym dsym_get(const char* name);
const char* dsym_reverse_get(Dsym dsym);
const char* ssym_reverse_get(Value value);

extern Dsym dsym_plus,  dsym_minus,  dsym_star,  dsym_slash;
extern Dsym dsym_plus2, dsym_minus2, dsym_star2, dsym_slash2;
extern Dsym dsym_gt, dsym_gt2;
extern Dsym dsym_lt, dsym_lt2;
extern Dsym dsym_gte, dsym_gte2;
extern Dsym dsym_lte, dsym_lte2;

//////////////////////////////////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////////////////////////////////
// Rules are the following: Anything can inherit from a VIRTUAL_OBJECT.
// VIRTUAL_OBJECT can also inherit anyone. FIELD_OBJECT may inherit another
// FIELD_OBJECT. No other combinations are allowed.
typedef enum {
  KLASS_DIRECT,
  KLASS_VIRTUAL_OBJECT,
  KLASS_CDATA_OBJECT,
  KLASS_FIELD_OBJECT
} KlassType;

typedef struct {
  Dsym name;
  KlassType type;
  Dict methods;
  int num_fields;
  int cdata_size;
  Dict readable_fields;
  Dict writable_fields;
  Dict fields;
  int obj_size;
} Klass;

//////////////////////////////////////////////////////////////////////////////
// common.c
//////////////////////////////////////////////////////////////////////////////
extern int sys_argc;
extern char** sys_argv;

extern Klass* klass_Nil;
extern Klass* klass_False;
extern Klass* klass_True;
extern Klass* klass_Eof;
extern Klass* klass_Integer;
extern Klass* klass_Double;
extern Klass* klass_Array1;
extern Klass* klass_Range;
extern Klass* klass_String;
extern Klass* klass_Tuple;

extern Dsym dsym_to_string;

void common_init_phase15();

//////////////////////////////////////////////////////////////////////////////
// stack.c
//////////////////////////////////////////////////////////////////////////////

#include <setjmp.h>

void stack_init();
void stack_push(Value func);
void stack_push_catch_all();
void stack_push_catch(Klass* exc_type);
void stack_pop();

#define exc_register_catch_all()   ({ \
                                     int _tmp = setjmp(exc_jb); \
                                     stack_push_catch_all(); \
                                     _tmp; \
                                   })
jmp_buf exc_jb; // TODO: Make this thread safe.
void exc_raise(char* format, ...) __attribute__ ((noreturn)) ;

//////////////////////////////////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////////////////////////////////

typedef struct {
  Klass* klass;
  Value values[0];
} Object;

Klass* klass_new(Dsym name, Dsym parent, KlassType type, int cdata_size);
void klass_new_method(Klass* klass, Dsym name, Value method);

#define FIELD_READABLE 1
#define FIELD_WRITABLE 2
int klass_new_field(Klass* klass, Dsym name, int type);
int klass_get_field_int(Klass* klass, Dsym name);
void klass_new_virtual_reader(Klass* klass, Dsym name, Value func);
void klass_new_virtual_writer(Klass* klass, Dsym name, Value func);
Klass* klass_get(Dsym name);
static inline const char* klass_name(Klass* klass){
  return dsym_reverse_get(klass->name);
}
void klass_init();
void klass_init_phase15();
void klass_dump();

// Used to initialize KLASS_OBJECT object.
Value obj_new(Klass* klass, void** data);
// Verify an object is of given type.
static inline Klass* obj_klass(Value v_obj)
{
  Value v_masked = v_obj & MASK_LONGTAIL;
  switch(v_masked){
    case 0b0000:
      if (v_masked == v_obj) return klass_Nil;
      return *((Klass**) unpack_ptr(v_obj));
    case 0b0100:
      if (v_masked == v_obj) return klass_False;
      return *((Klass**) unpack_ptr(v_obj));
    case 0b1000:
      if (v_masked == v_obj) return klass_True;
      return *((Klass**) unpack_ptr(v_obj));
    case 0b1100:
      if (v_masked == v_obj) return klass_Eof;
      return *((Klass**) unpack_ptr(v_obj));
    case 0b0001:
    case 0b0101:
    case 0b1001:
    case 0b1101:
      return klass_Integer;
    case 0b0010:
    case 0b0110:
    case 0b1010:
    case 0b1110:
      return klass_Double;
    case 0b0011:
    case 0b0111:
    case 0b1011:
    case 0b1111:
      assert_never();
  }
  assert_never();
}
static inline void obj_verify(Value v_obj, Klass* klass){
  Klass* klass_obj = obj_klass(v_obj);
  if (klass_obj != klass){
    exc_raise("TypeError: expected a %s, got a %s",
              dsym_reverse_get(klass->name),
              dsym_reverse_get(klass_obj->name));
  }
}
static inline void* obj_c_data(Value v_obj)
{
  return &(((Object*) unpack_ptr(v_obj))->values[0]);
}

Value field_get(Value v_obj, Dsym field);
void field_set(Value v_obj, Dsym field, Value val);

void method_error(Klass* klass, Dsym dsym);
static inline Value method_get(Value v_obj, Dsym dsym)
{
  Klass* klass = obj_klass(v_obj);
  Value method;
  if (dict_query(&(klass->methods), &dsym, &method)){
    return method;
  } else {
    method_error(klass, dsym);
    return VALUE_NIL;
  }
}

//////////////////////////////////////////////////////////////////////////////
// ops.c
//////////////////////////////////////////////////////////////////////////////
int64 op_hash(Value v);
Value op_equal(Value a, Value b);
bool op_equal2(Value a, Value b);
Value op_not_equal(Value a, Value b);
Value op_unary_not(Value v);
Value op_unary_minus(Value v);
Value op_and(Value a, Value b);
Value op_or(Value a, Value b);
#include "vm/ops-generated.h"

//////////////////////////////////////////////////////////////////////////////
// Arrays.c
//////////////////////////////////////////////////////////////////////////////

typedef struct {
  uint64 alloc_size;
  uint64 size;
  Value* data;
} Array1;
Array1* val_to_array1(Value array1);
Value array1_to_val(int64 num_elements, Value* data);
Value array1_to_val2(uint16 num_args, ...);
void array1_index_set(Array1* array1, int64 idx, Value val);
Value array1_index(Array1* array1, int64 idx);
Value array1_new(int64 num_elements);

extern Klass* klass_Array2;
typedef struct {
  uint64 size_x;
  uint64 size_y;
  Value* data;
} Array2;
extern Klass* klass_Array3;
typedef struct {
  uint64 size_x;
  uint64 size_y;
  uint64 size_z;
  Value* data;
} Array3;
Value array2_index(Value v_array, int64 x, int64 y);
void array2_index_set(Value v_array, int64 x, int64 y, Value v_val);
void init1_Arrays();
void init2_Arrays();

//////////////////////////////////////////////////////////////////////////////
// Complex.c
//////////////////////////////////////////////////////////////////////////////
extern Klass* klass_Complex;
typedef struct {
  double real;
  double imag;
} Complex;
void val_to_complex_soft(Value v, double* real, double* imag);
Value complex_to_val(double real, double imag);
Complex* val_to_complex(Value v);
void init1_Complex();
void init2_Complex();

//////////////////////////////////////////////////////////////////////////////
// Double.c
//////////////////////////////////////////////////////////////////////////////
#define double_to_val(x) pack_double(x)
#define val_to_double(v)  ({ obj_verify(v, klass_Double); unpack_double(v); })
double val_to_double_soft(Value v);

//////////////////////////////////////////////////////////////////////////////
// Function.c
//////////////////////////////////////////////////////////////////////////////
extern Klass* klass_func;
void init1_Function();
void init2_Function();
#include "vm/func-generated.h"
void func_set_vararg(Value v_func);

//////////////////////////////////////////////////////////////////////////////
// HashTable.c
//////////////////////////////////////////////////////////////////////////////
typedef enum {
  BUCKET_EMPTY = 0,
  BUCKET_WAS_FULL,
  BUCKET_FULL
} BucketType;

typedef struct {
  uint64 size;
  uint64 alloc_size;
  BucketType* buckets;
  Value* keys;
  Value* values;
} HashTable;

bool ht_query(HashTable* ht, Value key);
bool ht_query2(HashTable* ht, Value key, Value* value);
void ht_set(HashTable* ht, Value key);
void ht_set2(HashTable* ht, Value key, Value value);
bool ht_remove(HashTable* ht, Value key);
void ht_init(HashTable* ht);
void ht_init2(HashTable* ht);

//////////////////////////////////////////////////////////////////////////////
// Integer.c
//////////////////////////////////////////////////////////////////////////////
void init1_Integer();
void init2_Integer();
#define int64_to_val(a) pack_int64(a)
#define val_to_int64(v)  ({ obj_verify(v, klass_Integer); unpack_int64(v); })
int64 val_to_int64_soft(Value v);

//////////////////////////////////////////////////////////////////////////////
// Object.c
//////////////////////////////////////////////////////////////////////////////
void init1_Object();
void init2_Object();

//////////////////////////////////////////////////////////////////////////////
// Range.c
//////////////////////////////////////////////////////////////////////////////
typedef enum {
  RANGE_BOUNDED,
  RANGE_BOUNDED_LEFT,
  RANGE_BOUNDED_RIGHT,
  RANGE_UNBOUNDED
} RangeType;
typedef struct {
  int64 start;
  int64 finish;
  RangeType type;
} Range;
Value range_to_val(RangeType type, int64 start, int64 finish);
Range* val_to_range(Value range);

//////////////////////////////////////////////////////////////////////////////
// String.c
//////////////////////////////////////////////////////////////////////////////
typedef struct {
  char* str;
} String;

char* val_to_string(Value v);
Value string_to_val(char* str);

//////////////////////////////////////////////////////////////////////////////
// Tuple.c
//////////////////////////////////////////////////////////////////////////////
typedef struct {
  int64 size;
  Value* data;
} Tuple;
Tuple* val_to_tuple(Value v_tuple);
Value tuple_to_val(uint16 num_args, ...);
Value tuple_index(Tuple* tuple, int64 idx);
void tuple_index_set(Tuple* tuple, int64 idx, Value val);

//////////////////////////////////////////////////////////////////////////////
// util.c
//////////////////////////////////////////////////////////////////////////////
int64 util_index(const char* klass_name, int64 idx, int64 size);
void util_index_range(const char* klass_name, Range* range, int64 size,
                      int64* start, int64* finish);
const char* to_string(Value v);

#endif
