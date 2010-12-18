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

#include "namespaces/modules.h"
#include <fftw3.h>

Klass klass_FftwPlan2D;
typedef struct {
  fftw_complex *in, *out;
  fftw_plan plan;
  int64 x, y;
} FftwPlan2D;

static Value ripe_fftw_plan2d_new(Value x, Value y, Value v_sign)
{
  Value v_self = obj_new(klass_FftwPlan2D);
  FftwPlan2D* self = obj_c_data(v_self);

  self->x = val_to_int64(x);
  self->y = val_to_int64(y);
  if (self->x < 1 or self->y < 1){
    exc_raise("invalid dimensions of Fftw.Plan2D (%" PRId64"x%" PRId64")",
              self->x, self->y);
  }
  self->in = (fftw_complex*) fftw_malloc(
                               sizeof(fftw_complex) * self->x * self->y
                             );
  self->out = (fftw_complex*) fftw_malloc(
                                sizeof(fftw_complex) * self->x * self->y
                              );
  
  int64 sign = val_to_int64(v_sign);
  if (sign != FFTW_FORWARD and sign != FFTW_BACKWARD){
    exc_raise("invalid flag for FFT (passed %" PRId64")", sign);
  }

  self->plan = fftw_plan_dft_2d(self->x, self->y, self->in, self->out,
                                sign, FFTW_ESTIMATE);
  return v_self;
}

static Value ripe_fftw_plan2d_run(Value v_self, Value v_in, Value v_out)
{
  FftwPlan2D* self = obj_c_data(v_self);

  // Copy data from v_in
  for (int64 y = 0; y < self->y; y++){
    for (int64 x = 0; x < self->x; x++){
      Value v = array2_index(v_in, x+1, y+1);
      
      // 2 Possibilities, v is Double or Complex
      if (is_double(v)){
        self->in[x*self->y + y][0] = val_to_double(v);
        self->in[x*self->y + y][1] = 0.0;
      } else {
        Complex* complex = val_to_complex(v);
        self->in[x*self->y + y][0] = complex->real;
        self->in[x*self->y + y][1] = complex->imag;
      }
    }
  }
  
  fftw_execute(self->plan);
  
  // Copy data into v_out
  for (int64 y = 0; y < self->y; y++){
    for (int64 x = 0; x < self->x; x++){
      double real = self->out[x*self->y + y][0];
      double imag = self->out[x*self->y + y][1];

      Value v;
      if (imag == 0.0){
        v = double_to_val(real);
      } else {
        v = complex_to_val(real, imag);
      }
      array2_index_set(v_out, x+1, y+1, v);
    }
  }
  
  return VALUE_NIL;
}

static Value ripe_fftw_plan2d_get_kx(Value v_self, Value v_x)
{
  FftwPlan2D* self = obj_c_data(v_self);
  int64 x = val_to_int64(v_x) - 1;
  if (x > self->x / 2) x = x - self->x;
  return double_to_val(((double) x) / ((double) self->x));
}

static Value ripe_fftw_plan2d_get_ky(Value v_self, Value v_y)
{
  FftwPlan2D* self = obj_c_data(v_self);
  int64 y = val_to_int64(v_y) - 1;
  if (y > self->y / 2) y = y - self->y;
  return double_to_val(((double) y) / ((double) self->y));
}

void init_Fftw(int phase)
{
  if (phase == 1){
    klass_FftwPlan2D = klass_register("Fftw.Plan2D", sizeof(FftwPlan2D), 0);
  } else if (phase == 2) {
    ssym_set("Fftw.Plan2D.new", func3_to_val(ripe_fftw_plan2d_new));
    klass_register_method(klass_FftwPlan2D, "run", 
                                func3_to_val(ripe_fftw_plan2d_run));
    klass_register_method(klass_FftwPlan2D, "get_kx", 
                                func2_to_val(ripe_fftw_plan2d_get_kx));
    klass_register_method(klass_FftwPlan2D, "get_ky", 
                                func2_to_val(ripe_fftw_plan2d_get_ky));
  }
}

