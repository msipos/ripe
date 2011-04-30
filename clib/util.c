// Copyright (C) 2011  Maksim Sipos <msipos@mailc.net>
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
#include <unistd.h>

void err_throw(Error* e, const char* format, ...)
{
  char buf[10240];
  va_list ap;
  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);
  e->text = mem_strdup(buf);
}

static bool fatal_initialized = false;
static Array fatal_stack;
static void fatal_init()
{
  if (not fatal_initialized) {
    array_init(&fatal_stack, const char*);
    fatal_initialized = true;
  }
}

void fatal_push(const char* format, ...)
{
  fatal_init();

  char buf[10240];
  va_list ap;
  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  const char* s = mem_strdup(buf);
  array_append2(&fatal_stack, &s);
}

void fatal_pop()
{
  fatal_init();
  array_pop(&fatal_stack, const char*);
}

void fatal_vthrow(const char* format, va_list ap)
{
  fatal_init();

  for (int i = 0; i < fatal_stack.size; i++){
    const char* s;
    array_get2(&fatal_stack, &s, i);
    if (i == 0) fprintf(stderr, " ERROR: %s:\n", s);
           else fprintf(stderr, "        %s:\n", s);
  }
  if (fatal_stack.size > 0){
    fprintf(stderr, "        ");
  } else {
    fprintf(stderr, " ERROR: ");
  }
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

void fatal_throw(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  fatal_vthrow(format, ap);
}

void fatal_vwarn(const char* format, va_list ap)
{
  fatal_init();

  for (int i = 0; i < fatal_stack.size; i++){
    const char* s;
    array_get2(&fatal_stack, &s, i);
    if (i == 0) fprintf(stderr, " WARN: %s:\n", s);
           else fprintf(stderr, "       %s:\n", s);
  }
  if (fatal_stack.size > 0){
    fprintf(stderr, "        ");
  } else {
    fprintf(stderr, "  WARN: ");
  }
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

void fatal_warn(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  fatal_vwarn(format, ap);
}

#ifdef SLOG
void slog(const char* format, ...)
{
  char slog_filename[100];
  sprintf(slog_filename, "slog-%d", (int) getpid());
  FILE* f = fopen(slog_filename, "a");
  if (f == NULL){
    fprintf(stderr, "SLOG warning: failed to open '%s' for appending\n",
            slog_filename);
  }

  va_list ap;
  va_start(ap, format);
  vfprintf(f, format, ap);
  if (format[strlen(format) - 1] != '\n') fprintf(f, "\n");
  va_end(ap);
  fclose(f);
}
#endif

void encode_string(FILE* f, const char* s)
{
  if (s == NULL) s = "";
  if (strchr(s, '\n') != NULL){
    fatal_throw("attempted to encode a string with newlines ('%s')", s);
  }
  fprintf(f, "S: %s\n", s);
}

void encode_int(FILE* f, int64 i)
{
  fprintf(f, "I: %"PRId64"\n", i);
}

#define BUFSIZE 1024
static void decode(FILE* f, char* buf, char expected)
{
  if (fgets(buf, BUFSIZE, f) == NULL){
    fatal_throw("reading error while expecting '%c'", expected);
  }
  if (buf[1] != ':' or buf[2] != ' '){
    fatal_throw("broken serialization in line '%s'", buf);
  }
  if (buf[0] != expected){
    fatal_throw("unexpected serialization in line '%s' "
                "(expected '%c', got '%c')", buf, expected, buf[0]);
  }
  while (*buf != 0){
    if (*buf == '\n') *buf = 0;
    buf++;
  }
}

int64 decode_int(FILE* f)
{
  char buf[BUFSIZE]; decode(f, buf, 'I');
  return (int64) atol(buf+3);
}

const char* decode_string(FILE* f)
{
  char buf[BUFSIZE]; decode(f, buf, 'S');
  return mem_strdup(buf+3);
}
