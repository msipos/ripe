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

#ifndef UTF8_H
#define UTF8_H

#include "clib.h"

// Reads one unichar from the buffer given by *str, taking care
// not to read byte at limit or beyond it. Returns the unichar in
// out.
//
// If successful, it updates *str to point to the next codepoint (if any).
// If it fails, *str is left unchanged.
error utf8_read(const char** str, const char* limit, unichar* out);

// Write unichar c to the buffer given in *str, taking care not to write the
// byte at limit or beyond it.
//
// If successful, it updates *str_to point to the next codepoint (if any).
// If it fails, *str is left unchanged.
error utf8_write(char** str, const char* limit, unichar c);

#endif // UTF8_H
