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

Value range_to_val(RangeType type, int64 start, int64 finish)
{
  Range* range;
  Value obj = obj_new(klass_Range, (void**) &range);
  range->type = type;
  range->start = start;
  range->finish = finish;
  return obj;
}

Range* val_to_range(Value range)
{
  obj_verify(range, klass_Range);
  return obj_c_data(range);
}
