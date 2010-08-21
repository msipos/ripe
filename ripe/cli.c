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

#include <errno.h>
#include <string.h>
#include "clib/clib.h"
#include "ripe/ripe.h"

#define SEPARATOR '/'

#include <stdarg.h>
const char* path_join(int num, ...)
{
  va_list ap;
  va_start (ap, num);
  assert(num > 1);
  char* cur = va_arg(ap, char*);
  for (int i = 1; i < num; i++){
    char* next = va_arg(ap, char*);
    
    // Check that cur doesn't end with SEPARATOR
    int loc = strlen(cur) - 1;
    assert(loc > 0);
    if (cur[loc] == SEPARATOR) cur[loc] = 0;
    
    // Check that next doesn't start with SEPARATOR
    if (next[0] == SEPARATOR) next++;
    
    cur = mem_asprintf("%s%c%s", cur, SEPARATOR, next);
  }
  va_end (ap);
  return cur;
}

#include <unistd.h>
const char* path_get_app_dir()
{
  char buf[1024];
  size_t len;
  if ((len = readlink("/proc/self/exe", buf, 1024)) != -1){
    char* slash = strrchr(buf, SEPARATOR);
    if (slash != NULL){
      *slash = 0;
    }
    return mem_strdup(buf);
  } else {
    fprintf(stderr, "couldn't detect app directory: %s\n", 
            strerror(errno));
    return NULL;
  }
}

#include <sys/stat.h>
bool path_exists(const char* filename)
{
  struct stat buffer;
  int status;
  status = stat(filename, &buffer);
  if (status == -1) return false;
  return true;
}

const char* path_get_extension(const char* path)
{
  return strrchr(path, '.');
}

static char* strip_whitespace(char* s)
{
  // Go backwards and remove whitespace
  int l = strlen(s);
  for(;;){
    if (l == 0) break;
    char c = s[l-1];
    if (c == ' ' or c == '\t' or c== '\n'){
      s[l-1] = 0;
    } else {
      break;
    }
    l--;
  }

  // Go forwards and remove whitespace
  l = 0;
  for(;;){
    char c = s[l];
    if (c == 0) break;
    if (c == ' ' or c == '\t' or c== '\n'){
      l++;
    } else {
      break;
    }
  }

  return s + l;
}

#include <stdio.h>
void conf_load(Conf* conf, const char* filename)
{
  array_init(&(conf->keys), const char*);
  array_init(&(conf->values), const char*);
  if (not path_exists(filename)) return;

  FILE* f = fopen(filename, "r");
  if (f == NULL){
    fprintf(stderr, "warning: can't open file '%s' for reading: %s\n",
            filename, strerror(errno));
  }
  char line[1024];
  while (fgets(line, 1024, f) != NULL){
    char* p = strchr(line, '=');
    if (p == NULL) continue;
    *p = 0;
    char* key = strip_whitespace(line);
    char* value = strip_whitespace(p+1);
    // printf("key = '%s', value = '%s'\n", key, value);
    array_append(&(conf->keys), mem_strdup(key));
    array_append(&(conf->values), mem_strdup(value));
  }
}

#include <string.h>
const char* conf_query(Conf* conf, const char* key)
{
  for (int i = 0; i < conf->keys.size; i++){
    const char* tkey = array_get(&(conf->keys), char*, i);
    const char* tvalue = array_get(&(conf->values), char*, i);
    if (strcmp(key, tkey) == 0){
      return tvalue;
    }
  }
  return "";
}

