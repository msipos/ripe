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

#include <stdio.h>
#include <string.h>

#define MAX_PARAMS 10

const char* header = "// automatically generated by func-gen";

static void print_mult(const char* s, int n, int start_with_comma, int put_numbers)
{
  for (int i = 0; i < n; i++){
    if (i == 0){
      if (start_with_comma)
        printf(", ");
    } else {
      printf(", ");
    }
    if (put_numbers) {
      printf("%s%d", s, i + 1);
    } else {
      printf("%s", s);
    }
  }
}

static void gen_c(void)
{
  puts(header);
  printf("// func source code file\n\n");

  printf("#include \"vm/vm.h\"\n\n");

  printf(
    "static Value func_call_opt_helper("
    "Func* func, int16 num_args, Value* args)\n"
    "{\n"
    "  // -1 is for the catch-all parameter\n"
    "  int16 num_params = func->num_params;\n"
    "  int16 num_reqs = num_params - 1;\n"
    "  int16 num_opt = num_args - num_reqs;\n"
    "\n"
    "  if (num_reqs > num_args){\n"
    "    exc_raise(\"function that requires %%d arguments called with %%d\"\n"
    "              \" arguments\", num_reqs, num_args);\n"
    "  }\n"
    "  // Generate catch-all array\n"
    "  Value array = tuple_to_val2(num_opt, args + num_reqs);\n"
    "  switch(num_params){\n"
  );
  for (int n = 1; n <= MAX_PARAMS; n++){
    printf("    case %d:\n", n);
    printf("      return func->func%d(", n);
    for (int i = 0; i < n - 1; i++){
      printf("args[%d], ", i);
    }
    printf("array);\n");
  }
  printf("  }\n"
         "  assert_never();\n"
         "  return VALUE_NIL;\n"
         "}\n");

  printf("\n// callers\n");
  for (int n = 0; n <= MAX_PARAMS; n++){
    printf("Value func_call%d(Value func", n);
    print_mult("Value arg", n, 1, 1);
    printf("){\n");
    printf("  obj_verify(func, klass_Function);\n");
    printf("  Func* c_data = obj_c_data(func);\n");
    printf("  if (c_data->is_block){\n");

    if (n < MAX_PARAMS){
      printf("    return c_data->func%d(func", n+1);
      print_mult("arg", n, 1, 1);
      printf(");\n");
    } else {
      printf("    assert_never();\n");
      printf("    return VALUE_NIL;\n");
    }
  
    printf("  } else {\n");

    printf("    return c_data->func%d(", n);
    print_mult("arg", n, 0, 1);
    printf(");\n");

    printf("  }\n");
    printf("}\n");
  }

  for (int n = 0; n < MAX_PARAMS; n++){
    printf("Value method_call%d(Value v_obj, Value dsym", n);
    print_mult("Value arg", n, 1, 1);
    printf("){\n");
    printf("  Value method = method_get(v_obj, dsym);\n");
    printf("  Func* c_data = obj_c_data(method);\n");

    printf("  if (c_data->var_params){\n");
    printf("    Value args[%d] = {v_obj", n+1);
    print_mult("arg", n, 1, 1);
    printf("};\n");
    printf("    Value rv = func_call_opt_helper(c_data, %d, args);\n", n+1);
    printf("    return rv;\n");
    printf("  }\n");
    printf("  if (c_data->num_params != %d){\n", n+1);
    printf("    exc_raise(\"method that takes %%d arguments called with %%d\"\n");
    printf("              \" arguments\", c_data->num_params-1, %d);\n", n);
    printf("  }\n");
    printf("  Value rv = c_data->func%d(v_obj", n+1);
    print_mult("arg", n, 1, 1);
    printf(");\n");
    printf("  return rv;\n");
    printf("}\n");

  }
}

static void gen_h(void)
{
  puts(header);
  printf("// func header file\n\n");

  printf("#ifndef FUNC_GENERATED_H\n");
  printf("#define FUNC_GENERATED_H\n\n");

  printf("#include \"vm/vm.h\"\n\n");

  printf("// typedefs\n\n");
  for (int i = 0; i <= MAX_PARAMS; i++){
    printf("typedef Value (*CFunc%d) (", i);
    if (i == 0) printf("void");
    print_mult("Value", i, 0, 0);
    printf(");\n");
  }

  printf("// Func c_data\n");
  printf("typedef struct {\n");
  printf("  union {\n");
  printf("    void* func;\n");
  for (int i = 0; i <= MAX_PARAMS; i++){
    printf("    CFunc%d func%d;\n", i, i);
  }
  printf("  };\n  uint16 num_params;\n  uint16 var_params;\n  bool is_block;\n"
         "  uint16 block_elems;\n  Value* block_data;\n} Func;\n");

  printf("\n// constructors\n\n");
  for (int i = 0; i <= MAX_PARAMS; i++){
    printf("#define func%d_to_val(cfunc) "
           "func_to_val((void*) ((CFunc%d*) cfunc), %d)\n",
           i, i, i);
  }

  printf("\n// callers\n\n");
  for (int i = 0; i <= MAX_PARAMS; i++){
    printf("Value func_call%d(Value", i);
    print_mult("Value", i, 1, 0);
    printf(");\n");
  }
  for (int i = 0; i < MAX_PARAMS; i++){
    printf("Value method_call%d(Value v_obj, Value dsym", i);
    print_mult("Value arg", i, 1, 1);
    printf(");\n");
  }

  printf("\n#endif\n");
}

int main(int argc, char** argv)
{
  if (argc != 2) return 1;
  if (strcmp(argv[1], "c") == 0) gen_c();
  if (strcmp(argv[1], "h") == 0) gen_h();
  return 0;
}
