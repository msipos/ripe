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

Klass* klass_Complex;
static Value ripe_complex_get_real(Value v_self)
{
  Complex* self = obj_c_data(v_self);
  return double_to_val(self->real);
}

static Value ripe_complex_get_imag(Value v_self)
{
  Complex* self = obj_c_data(v_self);
  return double_to_val(self->imag);
}

static Value ripe_complex_new(Value v_real, Value v_imag)
{
  return complex_to_val(val_to_double(v_real), val_to_double(v_imag));
}

static Value ripe_complex_to_string(Value v_self)
{
  Complex* self = obj_c_data(v_self);
  char buf[128];
  sprintf(buf, "%g+%gi", self->real, self->imag);
  return string_to_val(buf);
}

static Value ripe_complex_slash(Value v_self, Value v_o)
{
  Complex* self = obj_c_data(v_self);
  double o = val_to_double_soft(v_o);
  return complex_to_val(self->real / o, self->imag / o);
}

static Value ripe_complex_star(Value v_self, Value v_o)
{
  Complex* self = obj_c_data(v_self);
  double o = val_to_double_soft(v_o);
  return complex_to_val(self->real * o, self->imag * o);
}

static Value ripe_complex_plus(Value v_self, Value v_o)
{
  Complex* self = obj_c_data(v_self);
  double o_re, o_im;
  val_to_complex_soft(v_o, &o_re, &o_im);
  return complex_to_val(self->real + o_re, self->imag + o_im);
}

static Value ripe_complex_minus(Value v_self, Value v_o)
{
  Complex* self = obj_c_data(v_self);
  double o_re, o_im;
  val_to_complex_soft(v_o, &o_re, &o_im);
  return complex_to_val(self->real - o_re, self->imag - o_im);
}

static Value ripe_complex_minus2(Value v_self, Value v_o)
{
  Complex* self = obj_c_data(v_self);
  double o_re, o_im;
  val_to_complex_soft(v_o, &o_re, &o_im);
  return complex_to_val(o_re - self->real, o_im - self->imag);
}

void val_to_complex_soft(Value v, double* real, double* imag)
{
  if (is_double(v)){
    *real = val_to_double(v);
    *imag = 0.0;
    return;
  }
  if (is_int64(v)){
    *real = val_to_double_soft(v);
    *imag = 0.0;
    return;
  }
  Complex* c = val_to_complex(v);
  *real = c->real;
  *imag = c->imag;
}

static Value ripe_to_real(Value v)
{
  if (is_int64(v)) return v;
  if (is_double(v)) return v;
  Complex* c = val_to_complex(v);
  return double_to_val(c->real);
}

Value complex_to_val(double real, double imag)
{
  Complex* complex;
  Value v_self = obj_new(klass_Complex, (void**) &complex);
  complex->real = real;
  complex->imag = imag;
  return v_self;
}

Complex* val_to_complex(Value v)
{
  obj_verify(v, klass_Complex);
  return obj_c_data(v);
}

void init1_Complex()
{
  klass_Complex = klass_new(dsym_get("Complex"),
                            dsym_get("Object"),
                            KLASS_CDATA_OBJECT,
                            sizeof(Complex));
  ssym_set("Complex.new", func2_to_val(ripe_complex_new));
  ssym_set("to_real", func1_to_val(ripe_to_real));
  klass_new_method(klass_Complex, dsym_get("get_real"),
                        func1_to_val(ripe_complex_get_real));
  klass_new_method(klass_Complex, dsym_get("get_imag"),
                        func1_to_val(ripe_complex_get_imag));
  klass_new_method(klass_Complex, dsym_get("to_s"),
                        func1_to_val(ripe_complex_to_string));
  klass_new_method(klass_Complex, dsym_get("__slash"),
                        func2_to_val(ripe_complex_slash));
  klass_new_method(klass_Complex, dsym_get("__star"),
                        func2_to_val(ripe_complex_star));
  klass_new_method(klass_Complex, dsym_get("__star2"),
                        func2_to_val(ripe_complex_star));
  klass_new_method(klass_Complex, dsym_get("__plus"),
                        func2_to_val(ripe_complex_plus));
  klass_new_method(klass_Complex, dsym_get("__plus2"),
                        func2_to_val(ripe_complex_plus));
  klass_new_method(klass_Complex, dsym_get("__minus"),
                        func2_to_val(ripe_complex_minus));
  klass_new_method(klass_Complex, dsym_get("__minus2"),
                        func2_to_val(ripe_complex_minus2));
}

void init2_Complex()
{
}
