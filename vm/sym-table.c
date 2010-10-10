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

#include "clib/dict.h"
#include "vm/vm.h"

Dict static_sym_table;
Dict static_sym_rev_table;
Dict dynamic_sym_table;
Dict dynamic_sym_rev_table;
Value dsym_plus, dsym_minus, dsym_star, dsym_slash;
Value dsym_plus2, dsym_minus2, dsym_star2, dsym_slash2;
Value dsym_gt, dsym_gt2;
Value dsym_lt, dsym_lt2;
Value dsym_gte, dsym_gte2;
Value dsym_lte, dsym_lte2;

void sym_init()
{
  dict_init(&static_sym_table, sizeof(char*), sizeof(Value),
            dict_hash_string, dict_equal_string);
  dict_init(&static_sym_rev_table, sizeof(Value), sizeof(char*),
            dict_hash_uint64, dict_equal_uint64);
  dict_init(&dynamic_sym_table, sizeof(char*), sizeof(Value),
            dict_hash_string, dict_equal_string);
  dict_init(&dynamic_sym_rev_table, sizeof(Value), sizeof(char*),
            dict_hash_uint64, dict_equal_uint64);
  dsym_plus = dsym_get("__plus"); dsym_plus2 = dsym_get("__plus2");
  dsym_minus = dsym_get("__minus"); dsym_minus2 = dsym_get("__minus2");
  dsym_star = dsym_get("__star"); dsym_star2 = dsym_get("__star2");
  dsym_slash = dsym_get("__slash"); dsym_slash2 = dsym_get("__slash2");
  dsym_gt = dsym_get("__gt"); dsym_gt2 = dsym_get("__gt2");
  dsym_gte = dsym_get("__gte"); dsym_gte2 = dsym_get("__gte2");
  dsym_lt = dsym_get("__lt"); dsym_lt2 = dsym_get("__lt2");
  dsym_lte = dsym_get("__lte"); dsym_lte2 = dsym_get("__lte2");
}

Value ssym_get(const char* name)
{
  Value val;
  if (dict_query(&static_sym_table, &name, &val)){
    return val;
  }
  exc_raise("required a missing symbol '%s'", name);
}

Value ssym_set(const char* name, Value val)
{
  if (dict_query(&static_sym_table, &name, NULL)) {
    exc_raise("defining a symbol '%s' that was already defined", name);
  }
  dict_set(&static_sym_table, &name, &val);
  dict_set(&static_sym_rev_table, &val, &name);
  return val;
}

Value dsym_get(const char* name)
{
  static int64 counter = 100;
  Value tmp;
  if (dict_query(&dynamic_sym_table, &name, &tmp)){
    return tmp;
  }
  counter++;
  tmp = pack_int64(counter);
  dict_set(&dynamic_sym_table, &name, &tmp);
  dict_set(&dynamic_sym_rev_table, &tmp, &name);
  return tmp;
}

const char* dsym_reverse_get(Value dsym)
{
  char* name;
  if (dict_query(&dynamic_sym_rev_table, &dsym, &name)){
    return name;
  }
  assert_never();
}

const char* ssym_reverse_get(Value value)
{
  char* name;
  if (dict_query(&static_sym_rev_table, &value, &name)){
    return name;
  }
  return NULL;
}
