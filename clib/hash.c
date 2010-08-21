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

#include "hash.h"

uint64 hash_bytes(const uint8* bytes, uint64 size, uint64 hash)
{
  while (size > sizeof(uint64))
  {
    // Simple hash.
    hash = *((const uint64*) bytes) + hash;
    hash = (hash >> 5) ^ (*((const uint64*) bytes));
    // Adapted from http://www.cris.com/~ttwang/tech/inthash.htm
    hash = (~hash) + (hash << 21); // hash = (hash << 21) - hash - 1;
    hash = hash ^ (hash >> 24);
    hash = (hash + (hash << 3)) + (hash << 8); // hash * 265
    hash = hash ^ (hash >> 14);
    hash = (hash + (hash << 2)) + (hash << 4); // hash * 21
    hash = hash ^ (hash >> 28);
    hash = hash + (hash << 31);
    bytes += sizeof(uint64);
    size -= sizeof(uint64);
  }
  memcpy(&size, bytes, size);
  hash = size + hash;
  hash = (hash >> 7) ^ size;
  hash = (~hash) + (hash << 21); // hash = (hash << 21) - hash - 1;
  hash = hash ^ (hash >> 24);
  hash = (hash + (hash << 3)) + (hash << 8); // hash * 265
  hash = hash ^ (hash >> 14);
  hash = (hash + (hash << 2)) + (hash << 4); // hash * 21
  hash = hash ^ (hash >> 28);
  hash = hash + (hash << 31);
  return (hash << 31) + hash;
}

