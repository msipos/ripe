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

#include "clib/clib.h"

// Some bit-sequence macros (to make this code more maintainable).
#define B_00000111 0x7
#define B_00001111 0xF
#define B_00011111 0x1F
#define B_00111111 0x3F
#define B_10000000 0x80
#define B_11000000 0xC0
#define B_11100000 0xE0
#define B_11110000 0xF0
#define B_11111000 0xF8
#define B_00000111_11000000 0x7C0
#define B_00001111_11000000 0xFC0
#define B_11110000_00000000 0xF000
#define B_00011100_00000000_00000000 0x1C0000
#define B_00000011_11110000_00000000 0x3F000

// Tells you if the given octet is in the ascii range (Unicode
// 00 through 7F).
#define utf8_is_ascii(c) (!(B_10000000 & (c)))

// Tells you if the given octet is in the middle of a UTF-8 encoded
// character.
#define utf8_in_middle(c) (((c) & B_11000000) == B_10000000)

// Tells you if the given octet is starting a 2-character UTF-8
// encoded sequence (Unicode 0080 through 07FF).
#define utf8_is_start2(c) (((c) & B_11100000) == B_11000000)

// Tells you if the given octet is starting a 3-character UTF-8
// encoded sequence (Unicode 0800 through D7FF and E000 through FFFF).
#define utf8_is_start3(c) (((c) & B_11110000) == B_11100000)

// Tells you if the given octet is starting a 4-character UTF-8
// encoded sequence (Unicode 010000 through 10FFFF).
#define utf8_is_start4(c) (((c) & B_11111000) == B_11110000)

// Tells you how many octets are needed to encode the given unichar u.
// This assumes a valid unichar (utf8_valid()).
#define utf8_needs(u) (((u) <= 0x7F) ? 1 : \
                        (((u) <= 0x7FF) ? 2 : \
                          (((u) <= 0xFFFF) ? 3 : 4) \
                        ) \
                      )

// Verifies if a unichar is not bigger than the allowed Unicode
// range. This does not check for the Unicode range D800 through DFFF,
// which is not allowed (but correctly encoded using UTF-8).
#define utf8_valid(u) ((u) <= 0x10FFFF)

error utf8_read(const char** str, const char* limit, unichar* out)
{
  // TODO: Current implementation seems clear, yet inefficient. Eventually
  // it should be improved.
  assert(str != NULL); assert((*str) != NULL); assert(out != NULL);

  // 4 byte octets.
  const char* str1 = *str;
  const char* str2 = str1 + 1;
  const char* str3 = str1 + 2;
  const char* str4 = str1 + 3;

  // These will be used to construct parts of out from the individual
  // octets.
  unichar w, x, y, z;

  // Few sanity checks:
  if (str1 >= limit) return ERROR_UTF8_LIMITED;
  if (utf8_in_middle(*str1)) return ERROR_UTF8_SEEK;

  if (utf8_is_ascii(*str1)) {
    // 1 octet: 0zzzzzzz => 0zzzzzzz
    (*str)++;
    *out = (unichar) (*str1);
    return ERROR_OK;
  } else if (utf8_is_start2(*str1)) {
    // 2 octets: 110yyyyy 10zzzzzz => yyyyyzzzzzz
    if (str2 >= limit) return ERROR_UTF8_LIMITED;
    if (not utf8_in_middle(*str2)) return ERROR_UTF8_INVALID;

    y = (*str1) & B_00011111;
    z = (*str2) & B_00111111;
    (*str) += 2;
    *out = (y << 6) | z;
    return ERROR_OK;
  } else if (utf8_is_start3(*str1)) {
    // 3 octets: 1110xxxx 10yyyyyy 10zzzzzz => xxxxyyyy yyzzzzzz
    if (str3 >= limit) return ERROR_UTF8_LIMITED;
    if (not utf8_in_middle(*str2)) return ERROR_UTF8_INVALID;
    if (not utf8_in_middle(*str3)) return ERROR_UTF8_INVALID;

    x = (*str1) & B_00001111;
    y = (*str2) & B_00111111;
    z = (*str3) & B_00111111;
    (*str) += 3;
    *out = (x << 12) | (y << 6) | z;
    return ERROR_OK;
  } else if (utf8_is_start4(*str1)) {
    // 4 octets: 11110www 10xxxxxx 10yyyyyy 10zzzzzz => wwwxx xxxxyyyy yyzzzzzz
    if (str4 >= limit) return ERROR_UTF8_LIMITED;
    if (not utf8_in_middle(*str2)) return ERROR_UTF8_INVALID;
    if (not utf8_in_middle(*str3)) return ERROR_UTF8_INVALID;
    if (not utf8_in_middle(*str4)) return ERROR_UTF8_INVALID;

    w = (*str1) & B_00000111;
    x = (*str2) & B_00111111;
    y = (*str3) & B_00111111;
    z = (*str4) & B_00111111;
    (*str) += 4;
    *out = (w << 18) | (x << 12) | (y << 6) | z;
    return ERROR_OK;
  }
  return ERROR_UTF8_INVALID;
}

error utf8_write(char** str, const char* limit, unichar c)
{
  assert(str != NULL); assert((*str) != NULL);

  if (not utf8_valid(c)) return ERROR_UTF8_INVALID;
  int bytes = utf8_needs(c);

  // 4 byte octets.
  uint8* str1 = (uint8*) *str;
  uint8* str2 = str1 + 1;
  uint8* str3 = str1 + 2;
  uint8* str4 = str1 + 3;

  if (str1 + bytes > (uint8*) limit) return ERROR_UTF8_LIMITED;
  switch(bytes){
    case 1:
      *str1 = (uint8) c;
      (*str)++;
      break;
    case 2:
      // 2 octets: yyy yyzzzzzz => 110yyyyy 10zzzzzz
      *str1 = (uint8) (((B_00000111_11000000 & c) >> 6) | B_11000000);
      *str2 = (uint8) ((B_00111111 & c) | B_10000000);
      (*str) += 2;
      break;
    case 3:
      // 3 octets: xxxxyyyy yyzzzzzz => 1110xxxx 10yyyyyy 10zzzzzz
      *str1 = (uint8) (((B_11110000_00000000 & c) >> 12) | B_11100000);
      *str2 = (uint8) (((B_00001111_11000000 & c) >> 6) | B_10000000);
      *str3 = (uint8) ((B_00111111 & c) | B_10000000);
      (*str) += 3;
      break;
    case 4:
      // 4 octets: 000wwwxx xxxxyyyy yyzzzzzz
      //           => 11110www 10xxxxxx 10yyyyyy 10zzzzzz
      *str1 = (uint8) (((B_00011100_00000000_00000000 & c) >> 18) | B_11110000);
      *str2 = (uint8) (((B_00000011_11110000_00000000 & c) >> 12) | B_10000000);
      *str3 = (uint8) (((B_00001111_11000000 & c) >> 6) | B_10000000);
      *str4 = (uint8) ((B_00111111 & c) | B_10000000);
      (*str) += 4;
      break;
    default:
      assert_never();
  }
  return ERROR_OK;
}
