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
#include "clib/clib.h"
#include <stdarg.h>

const char* to_string(Value v)
{
  return val_to_string(
    method_call0(v, dsym_get("to_string"))
  );
}

uint64 hash(Value v)
{
  switch(v & MASK_TAIL){
    case 0b00:
      return (uint64) val_to_int64(method_call0(v, dsym_get("hash")));
    case 0b01:
    case 0b10:
    case 0b11:
      return (uint64) unpack_int64(v);
  }
  assert_never();
}

