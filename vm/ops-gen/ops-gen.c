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

const char* c_template_int =
"Value op_%s(Value a, Value b)\n"
"{\n"
"  switch(a & MASK_TAIL){\n"
"    case 0b00:\n"
"      return method_call1(a, dsym_%s, b);\n"
"    case 0b01:\n"
"      switch(b & MASK_TAIL){\n"
"        case 0b00:\n"
"          return method_call1(b, dsym_%s2, a);\n"
"        case 0b01:\n"
"          return pack_int64(unpack_int64(a) %s unpack_int64(b));\n"
"        case 0b10:\n"
"        default:\n"
"          goto error;\n"
"      }\n"
"    case 0b10:\n"
"      goto error;\n"
"  }\n"
"error:\n"
"  exc_raise(\"invalid operands of '%s' (%%s and %%s)\",\n"
"            klass_name(obj_klass(a)),\n"
"            klass_name(obj_klass(b)));\n"
"}\n";

const char* c_template =
"Value op_%s(Value a, Value b)\n"
"{\n"
"  switch(a & MASK_TAIL){\n"
"    case 0b00:\n"
"      return method_call1(a, dsym_%s, b);\n"
"    case 0b01:\n"
"      switch(b & MASK_TAIL){\n"
"        case 0b00:\n"
"          return method_call1(b, dsym_%s2, a);\n"
"        case 0b01:\n"
"          return pack_%s(unpack_int64(a) %s unpack_int64(b));\n"
"        case 0b10:\n"
"          return pack_%s(((double) unpack_int64(a)) %s unpack_double(b));\n"
"        default:\n"
"          goto error;\n"
"      }\n"
"    case 0b10:\n"
"      switch(b & MASK_TAIL){\n"
"        case 0b00:\n"
"          return method_call1(b, dsym_%s2, a);\n"
"        case 0b01:\n"
"          return pack_%s(unpack_double(a) %s ((double) unpack_int64(b)));\n"
"        case 0b10:\n"
"          return pack_%s(unpack_double(a) %s unpack_double(b));\n"
"        default:\n"
"          goto error;\n"
"      }\n"
"  }\n"
"error:\n"
"  exc_raise(\"invalid operands of '%s' (%%s and %%s)\",\n"
"            klass_name(obj_klass(a)),\n"
"            klass_name(obj_klass(b)));\n"
"}\n";

const char* h_template = "Value op_%s(Value a, Value b);\n";

void func_numeric_numeric(const char* name, const char* op){
  printf(c_template,
         name, name,
         name,
         "int64", op,
         "double", op,
         name,
         "double", op,
         "double", op,
         op);
}

void func_bool_numeric(const char* name, const char* op){
  printf(c_template,
         name, name,
         name,
         "bool", op,
         "bool", op,
         name,
         "bool", op,
         "bool", op,
         op);
}

void func_int_int(const char* name, const char* op)
{
  printf(c_template_int, name, name, name, op, name);
}

void gen_c()
{
  printf("// ops-generated source file\n\n");

  printf("#include \"vm/ops-generated.h\"\n\n");
  func_numeric_numeric("plus", "+");
  func_numeric_numeric("minus", "-");
  func_numeric_numeric("star", "*");
  func_numeric_numeric("slash", "/");
  func_bool_numeric("gt", ">");
  func_bool_numeric("gte", ">=");
  func_bool_numeric("lt", "<");
  func_bool_numeric("lte", "<=");
  func_int_int("bit_and", "&");
  func_int_int("bit_or", "|");
  func_int_int("bit_xor", "^");
  func_int_int("modulo", "%");
}

void gen_h()
{
  printf("// ops-generated header file\n\n");

  printf("#ifndef OPS_GENERATED_H\n");
  printf("#define OPS_GENERATED_H\n\n");

  printf("#include \"vm/vm.h\"\n\n");

  printf(h_template, "plus");
  printf(h_template, "minus");
  printf(h_template, "star");
  printf(h_template, "slash");
  printf(h_template, "gt");
  printf(h_template, "gte");
  printf(h_template, "lt");
  printf(h_template, "lte");
  printf(h_template, "bit_and");
  printf(h_template, "bit_or");
  printf(h_template, "bit_xor");
  printf(h_template, "modulo");
  printf("#endif\n");
}

int main(int argc, char** argv)
{
  if (argc != 2) return 1;
  if (strcmp(argv[1], "c") == 0) gen_c();
  if (strcmp(argv[1], "h") == 0) gen_h();
  return 0;
}
