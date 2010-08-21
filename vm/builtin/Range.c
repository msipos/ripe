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

/////////////////////////////////////////////////////////////////////////////
//       RangeIter
/////////////////////////////////////////////////////////////////////////////

Klass* klass_Range_iter;
typedef struct {
  int64 start;
  int64 finish;
  int64 idx;
  int64 step;
} RangeIterCData;

Value range_iter_to_val(int64 start, int64 finish)
{
  RangeIterCData* c_data;
  Value obj = obj_new(klass_Range_iter, (void**) &c_data);
  c_data->start = start;
  c_data->finish = finish;
  c_data->idx = start;
  if (start <= finish) c_data->step = 1;
    else c_data->step = -1;
  return obj;
}

Value ripe_range_iter_iter(Value self)
{
  RangeIterCData* c_data = obj_c_data(self);
  if (c_data->idx == (c_data->finish + c_data->step)) return VALUE_EOF;
  int64 rv = c_data->idx;
  c_data->idx += c_data->step;
  return int64_to_val(rv);
}

/////////////////////////////////////////////////////////////////////////////
//       Range
/////////////////////////////////////////////////////////////////////////////

Klass* klass_Range;
typedef struct {
  int64 start;
  int64 finish;
} RangeCData;

Value range_to_val(int64 start, int64 finish)
{
  RangeCData* c_data;
  Value obj = obj_new(klass_Range, (void**) &c_data);
  c_data->start = start;
  c_data->finish = finish;
  return obj;
}

void val_to_range(Value range, int64* start, int64* finish)
{
  assert(start != NULL); assert(finish != NULL);
  obj_verify(range, klass_Range);
  RangeCData* c_data = obj_c_data(range);
  *start = c_data->start;
  *finish = c_data->finish;
}

static Value ripe_to_string(Value self)
{
  int64 start, finish;
  val_to_range(self, &start, &finish);
  char buf[100];
  sprintf(buf, "%"PRId64":%"PRId64, start, finish);
  return string_to_val(buf);
}

static Value ripe_range_get_iter(Value self)
{
  int64 start, finish;
  val_to_range(self, &start, &finish);
  return range_iter_to_val(start, finish);
}

void init1_Range(){
  klass_Range = klass_new(dsym_get("Range"),
                          dsym_get("Object"),
                          KLASS_OBJECT,
                          sizeof(RangeCData));
  klass_new_method(klass_Range,
                   dsym_get("to_string"),
                   func1_to_val(ripe_to_string));
  klass_new_method(klass_Range,
                   dsym_get("get_iter"),
                   func1_to_val(ripe_range_get_iter));

  klass_Range_iter = klass_new(dsym_get("RangeIter"),
                               dsym_get("Object"),
                               KLASS_CDATA_OBJECT,
                               sizeof(RangeIterCData));
  klass_new_method(klass_Range_iter,
                   dsym_get("iter"),
                   func1_to_val(ripe_range_iter_iter));
}

void init2_Range()
{
}
