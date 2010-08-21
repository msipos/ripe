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

#include "modules/modules.h"


// Bilinear interpolation
static Value ripe_sci_interpolate2(Value v_field, Value v_x, Value v_y)
{
  klass_verify(v_field, klass_Array2);
  Array2* array2 = obj_c_data(v_field);
  int64 nx = array2->size_x;
  int64 ny = array2->size_y;

  int64 x = val_to_int64_soft(v_x);
  int64 y = val_to_int64_soft(v_y);
  //printf("%"PRId64", %"PRId64 "\n", x, y);
  double dx = val_to_double_soft(v_x) - x;
  double dy = val_to_double_soft(v_y) - y;

  if (x < 1) {
    x = 1; dx = 0.0;
  }
  if (y < 1) {
    y = 1; dy = 0.0;
  }

  // Special cases that boil down to linear interpolation
  if (x >= nx){
    if (y >= ny){
      return array2_index(v_field, nx, ny);
    } else {
      return double_to_val(
              (1.0 - dy) * val_to_double_soft(array2_index(v_field, nx, y))
                    + dy * val_to_double_soft(array2_index(v_field, nx, y+1))
             );
    }
  } else {
    if (y >= ny){
      return double_to_val(
              (1.0 - dx) * val_to_double_soft(array2_index(v_field, x, ny))
                   + dx * val_to_double_soft(array2_index(v_field, x+1, ny))
             );
    }
  }
  
  double d11 = val_to_double_soft(
                 array2_index(v_field, x, y));
  double d12 = val_to_double_soft(
                 array2_index(v_field, x, y+1));
  double d21 = val_to_double_soft(
                 array2_index(v_field, x+1, y));
  double d22 = val_to_double_soft(
                 array2_index(v_field, x+1, y+1));
  return double_to_val(
           (1.0 - dy)*( (1.0-dx)*d11 + dx*d21 )
                 + dy*( (1.0-dx)*d12 + dx*d22 )
         );
}

void init_Sci(int phase)
{
  switch (phase) {
    case 1:
      break;
    case 2:
      ssym_set("Sci.interpolate2", func3_to_val(ripe_sci_interpolate2));
      break;
    case 3:
      break;
  }
}
