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
void sym_init();
Value ssym_get(const char* name);
Value ssym_set(const char* name, Value val);
Value dsym_get(const char* name);
const char* dsym_reverse_get(Value dsym);
const char* ssym_reverse_get(Value value);

extern Value dsym_plus,  dsym_minus,  dsym_star,  dsym_slash;
extern Value dsym_plus2, dsym_minus2, dsym_star2, dsym_slash2;
extern Value dsym_gt, dsym_gt2;
extern Value dsym_lt, dsym_lt2;
extern Value dsym_gte, dsym_gte2;
extern Value dsym_lte, dsym_lte2;
extern Value dsym_bit_and, dsym_bit_or, dsym_bit_xor;
extern Value dsym_bit_and2, dsym_bit_or2, dsym_bit_xor2;
extern Value dsym_modulo, dsym_modulo2;

//////////////////////////////////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////////////////////////////////
#include "vm/func-generated.h"

struct KlassT {
  struct KlassT* parent;
  Value name;
  Dict methods;
  int num_fields;
  int cdata_size;
  Dict readable_fields;
  Dict writable_fields;
  Dict fields;
  int obj_size;
  CFunc1 destructor;
};
typedef struct KlassT Klass;

//////////////////////////////////////////////////////////////////////////////
// common.c
//////////////////////////////////////////////////////////////////////////////
extern int sys_argc;
extern char** sys_argv;

extern Klass* klass_Array1;
extern Klass* klass_Array3;
extern Klass* klass_Bool;
extern Klass* klass_Destroyed;
extern Klass* klass_Double;
extern Klass* klass_Eof;
extern Klass* klass_Error;
extern Klass* klass_Function;
extern Klass* klass_Integer;
extern Klass* klass_Map;
extern Klass* klass_Nil;
extern Klass* klass_Range;
extern Klass* klass_Set;
extern Klass* klass_String;
extern Klass* klass_Tuple;

extern Value dsym_contains;
extern Value dsym_destructor;
extern Value dsym_name;
extern Value dsym_text;
extern Value dsym_to_string;

void common_init_phase15();

//////////////////////////////////////////////////////////////////////////////
// stack.c
//////////////////////////////////////////////////////////////////////////////

#include <setjmp.h>

void stack_init();
void stack_display();
void exc_raise(char* format, ...) __attribute__ ((noreturn));
void exc_raise_object(Value obj) __attribute__ ((noreturn));

// Annotation stuff
void stack_annot_push(char* annotation);
void stack_annot_pop();
static inline Value stack_annot_pop_pass(Value stuff)
{
  stack_annot_pop();
  return stuff;
}
#define  RRETURN(x)  return stack_annot_pop_pass(x)

void stack_push_catch_all();
void stack_pop();
void stack_push_finally();
void stack_push_catch(Klass* exc_type);
void stack_continue_unwinding() __attribute__ ((noreturn));

extern THREAD_LOCAL jmp_buf exc_jb;
extern THREAD_LOCAL Value exc_obj;
extern THREAD_LOCAL bool stack_unwinding;

//////////////////////////////////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////////////////////////////////

typedef struct {
  Klass* klass;
  Value values[0];
} Object;

Klass* klass_new(Value name, int cdata_size);
void klass_new_method(Klass* klass, Value name, Value method);

#define FIELD_READABLE 1
#define FIELD_WRITABLE 2
int klass_new_field(Klass* klass, Value name, int type);
int klass_get_field_int(Klass* klass, Value name);
void klass_new_virtual_reader(Klass* klass, Value name, Value func);
void klass_new_virtual_writer(Klass* klass, Value name, Value func);
Klass* klass_get(Value name);
static inline const char* klass_name(Klass* klass){
  return dsym_reverse_get(klass->name);
}
void klass_init();
void klass_init_phase15();
void klass_dump();

Value obj_new(Klass* klass, void** data);
Value obj_new2(Klass* klass);
void obj_destroy(Value obj);

// Verify an object is of given type.
static inline Klass* obj_klass(Value v_obj)
{
  Value v_masked = v_obj & MASK_LONGTAIL;
  switch(v_masked){
    case 0b0000:
      if (v_masked == v_obj) return klass_Nil;
      return *((Klass**) unpack_ptr(v_obj));
    case 0b0100:
      if (v_masked == v_obj) return klass_Bool;
      return *((Klass**) unpack_ptr(v_obj));
    case 0b1000:
      if (v_masked == v_obj) return klass_Bool;
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
  // Crash in release build:
  return NULL;
}
static inline void obj_verify(Value v_obj, Klass* klass){
  Klass* klass_obj = obj_klass(v_obj);
  if (klass_obj != klass){
    exc_raise("TypeError: expected a %s, got a %s",
              dsym_reverse_get(klass->name),
              dsym_reverse_get(klass_obj->name));
  }
}
Value obj_verify_assign(Value v_obj, Klass* klass);

static inline void* obj_c_data(Value v_obj)
{
  return &(((Object*) unpack_ptr(v_obj))->values[0]);
}

static inline void** obj_c_dptr(Value v_obj)
{
  return obj_c_data(v_obj);
}

static inline void* obj_c_ptr(Value v_obj)
{
  return *((void**) obj_c_data(v_obj));
}

Value field_get(Value v_obj, Value field);
void field_set(Value v_obj, Value field, Value val);
bool field_has(Value v_obj, Value field);

void method_error(Klass* klass, Value dsym);
bool method_has(Value v_obj, Value dsym);
static inline Value method_get(Value v_obj, Value dsym)
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

bool obj_eq_klass(Value v_obj, Klass* k);

//////////////////////////////////////////////////////////////////////////////
// ops.c
//////////////////////////////////////////////////////////////////////////////
int64 op_hash(Value v);
Value op_equal(Value a, Value b);
bool op_equal2(Value a, Value b);
Value op_not_equal(Value a, Value b);
Value op_unary_not(Value v);
Value op_unary_minus(Value v);
Value op_unary_bit_not(Value v);
Value op_and(Value a, Value b);
Value op_or(Value a, Value b);
Value op_in(Value a, Value b);
Value op_exp(Value a, Value b);
#include "vm/ops-generated.h"

//////////////////////////////////////////////////////////////////////////////
// format.c
//////////////////////////////////////////////////////////////////////////////

typedef struct {
  int type;
  const char* str;
  int64 number;
} FormatParseElement;

typedef struct {
  int size;
  FormatParseElement* elements;
} FormatParse;

#define FORMAT_STRING  1
#define FORMAT_NUMBER  2

char* format_to_string(const char* fstr, uint64 num_values, Value* values);
int format_parse(const char* fstr, FormatParse* fp);

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
Value array1_to_val2(int num_args, ...);
void array1_index_set(Array1* array1, int64 idx, Value val);
Value array1_index(Array1* array1, int64 idx);
Value array1_new(int64 num_elements);
void array1_push(Array1* a, Value val);
Value array1_pop(Array1* a);

typedef struct {
  uint64 size_x;
  uint64 size_y;
  Value* data;
} Array2;
int64 array2_index(Array2* array, int64 x, int64 y);

typedef struct {
  uint64 size_x;
  uint64 size_y;
  uint64 size_z;
  Value* data;
} Array3;
uint64 array3_index(Array3* array3, uint64 x, uint64 y, uint64 z);

//////////////////////////////////////////////////////////////////////////////
// Complex.c
//////////////////////////////////////////////////////////////////////////////
extern Klass* klass_Complex;
typedef struct {
  double real;
  double imag;
} RipeComplex;
void val_to_complex_soft(Value v, double* real, double* imag);
//Value complex_to_val(double real, double imag);
//Complex* val_to_complex(Value v);

//////////////////////////////////////////////////////////////////////////////
// Double.c
//////////////////////////////////////////////////////////////////////////////
#define double_to_val(x) pack_double(x)
double val_to_double(Value v);

//////////////////////////////////////////////////////////////////////////////
// Function.c
//////////////////////////////////////////////////////////////////////////////
extern Klass* klass_func;
void init1_Function();
void init2_Function();
Value func_to_val(void* c_func, int num_params);
Value block_to_val(void* c_func, int num_params, int block_elems, ...);
void func_set_vararg(Value v_func);
void* func_get_ptr(Value v_func, int16 num_params);
// TODO: Change this if you want stack with optimizations
#define FUNC_CALL(f, ...)   f(__VA_ARGS__)

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
void ht_clear(HashTable* ht);
void ht_init(HashTable* ht, int64 items);
void ht_init2(HashTable* ht, int64 items);
Value ht_new_map(int64 num, ...);
Value ht_new_set(int64 num, ...);

//////////////////////////////////////////////////////////////////////////////
// Integer.c
//////////////////////////////////////////////////////////////////////////////
void init1_Integer();
void init2_Integer();
#define int64_to_val(a) pack_int64(a)
#define val_to_int64(v)  ({ obj_verify(v, klass_Integer); unpack_int64(v); })
int64 val_to_int64_soft(Value v);
// Put a pointer into an Integer.
static inline Value ptr_to_val(void* p)
{
  return (Value) ((uintptr) p) + 1;
}
static void* val_to_ptr(Value v)
{
  obj_verify(v, klass_Integer);
  return (void*) (((uintptr) v) - 1);
}
static inline void* val_to_ptr_unsafe(Value v)
{
  return (void*) (((uintptr) v) - 1);
}

//////////////////////////////////////////////////////////////////////////////
// Num.c
//////////////////////////////////////////////////////////////////////////////

#include <complex.h>
#undef I

#define NUM_DOUBLE   1
#define NUM_COMPLEX  2
#define NUM_INT      3
#define NUM_INT8     4
typedef struct {
  uint64 size;
  union {
    void* data;
    double* data_double;
    double complex* data_complex;
    int64* data_int64;
    int8* data_int8;
  };
  int type;
} NumArray1;

typedef struct {
  uint64 size_x;
  uint64 size_y;
  union {
    void* data;
    double* data_double;
    double complex* data_complex;
    int64* data_int64;
    int8* data_int8;
  };
  int type;
} NumArray2;
// Index should always be calculated via y*size_x + x
// And loops should always be y outside, x inside

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
int64 range_start(Value range);
int64 range_delta(Value range);
int64 range_finish(Value range);
Range* val_to_range(Value range);

//////////////////////////////////////////////////////////////////////////////
// String.c
//////////////////////////////////////////////////////////////////////////////
#define STRING_REGULAR  1
#define STRING_CONST    2
typedef struct {
  int type;
  char* str;
} String;

char* val_to_string(Value v);
Value string_to_val(const char* str);
Value stringn_to_val(const char* str, int n);
Value string_const_to_val(const char* str);

//////////////////////////////////////////////////////////////////////////////
// Tuple.c
//////////////////////////////////////////////////////////////////////////////
typedef struct {
  int64 size;
  Value* data;
} Tuple;
Tuple* val_to_tuple(Value v_tuple);
Value tuple_new(int64 size, Tuple** out);
Value tuple_to_val(uint16 num_args, ...);
Value tuple_to_val2(uint16 num_args, Value* stuff);
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
