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

// This file is included from value.h

// Value is a 64-bit value. It assumes that all pointers returned by malloc
// are aligned to 4 bytes (last 2 bits are 0).

// Containers:
//   Pointer
//     xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxx00
//   nil
//     00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
//   false
//     00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000100
//   true
//     00000000 00000000 00000000 00000000 00000000 00000000 00000000 00001000
//   eof
//     00000000 00000000 00000000 00000000 00000000 00000000 00000000 00001100
//   Integer
//     xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxx01
//   Double
//     xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxx10
//   Extended
//     xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxx11

// Specific values:
#define VALUE_NIL   0b0000
#define VALUE_FALSE 0b0100
#define VALUE_TRUE  0b1000
#define VALUE_EOF   0b1100

// Masks:
#define MASK_TAIL     ((Value) 0b11)
#define MASK_LONGTAIL ((Value) 0b1111)

static inline bool is_int64(Value v)
{
  return (v & MASK_TAIL) == 0b01;
}
static inline bool is_double(Value v)
{
  return (v & MASK_TAIL) == 0b10;
}
static inline bool is_ptr(Value v)
{
  return (v & MASK_TAIL) == 0b00 and (v & MASK_LONGTAIL) != v;
}

static inline Value pack_double(double d)
{
  Value v = *((uint64*) (&d));
  return v - (v & MASK_TAIL) + 0b10;
}
static inline double unpack_double(Value v)
{
  v = v - 0b10;
  return *((double*) (&v));
}

static inline Value pack_int64(int64 i)
{
  return (((Value) i) << 2) + 0b1;
}
static inline int64 unpack_int64(Value v)
{
  return ((int64) v) >> 2;
}

static inline Value pack_ptr(void* p)
{
  return (Value) ((uintptr) p);
}
static inline void* unpack_ptr(Value v)
{
  return (void*) ((uintptr) v);
}

static inline Value pack_bool(bool b)
{
  return b ? VALUE_TRUE : VALUE_FALSE;
}
static inline bool unpack_bool(Value v)
{
  return v == VALUE_TRUE ? true : false;
}
