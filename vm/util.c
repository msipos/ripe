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

// Map Ripe index to C array index (or throw an exception if it isn't valid.
int64 util_index(const char* klass_name, int64 idx, int64 size)
{
  if (idx > 0){
    if (idx > size) goto invalid_idx;
    return idx - 1;
  }
  if (idx == 0) goto invalid_idx;
  if (idx < -size) goto invalid_idx;
  return size + idx;
invalid_idx:
  exc_raise("invalid index %"PRId64" in %s of size %"PRId64,
            idx, klass_name, size);
}

// Map Ripe Range indices to C indices (or throw an exception)
void util_index_range(const char* klass_name, Range* range, int64 size,
                      int64* start, int64* finish)
{
  assert(start != NULL); assert(finish != NULL);
  switch(range->type){
    case RANGE_BOUNDED:
      *start = util_index(klass_name, range->start, size);
      *finish = util_index(klass_name, range->finish, size);
      return;
    case RANGE_BOUNDED_LEFT:
      *start = util_index(klass_name, range->start, size);
      *finish = size - 1;
      return;
    case RANGE_BOUNDED_RIGHT:
      *start = 0;
      *finish = util_index(klass_name, range->finish, size);
      return;
    case RANGE_UNBOUNDED:
      *start = 0;
      *finish = size - 1;
      return;
  }
}

const char* to_string(Value v)
{
  return val_to_string(
    method_call0(v, dsym_to_string)
  );
}
