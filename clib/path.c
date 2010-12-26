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
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SEPARATOR '/'

#include <sys/stat.h>
bool path_exists(const char* filename)
{
  struct stat buffer;
  int status;
  status = stat(filename, &buffer);
  if (status == -1) return false;
  return true;
}

const char* path_get_app_dir()
{
  char buf[1024];
  size_t len;
  if ((len = readlink("/proc/self/exe", buf, 1024)) != -1){
    buf[len] = 0; // readlink does not terminate buf with 0
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

const char* path_get_temp_dir()
{
  const char* t = getenv("TMPDIR");
  if (t != NULL) return mem_strdup(t);
  #ifdef P_tmpdir
  return mem_strdup(P_tmpdir);
  #endif
  return mem_strdup("/tmp");
}

const char* path_temp_name(const char* prefix, const char* suffix)
{
  char buf[16];
  FILE *f = fopen("/dev/urandom", "rb");
  for (int i = 0; i < 15; i++){
    uint16 r;
    fread(&r, sizeof(r), 1, f);
    if (r & 1) {
      buf[i] = 'a' + ((r >> 1) % 26);
    } else {
      buf[i] = 'A' + ((r >> 1) % 26);
    }
  }
  fclose(f);
  buf[16] = 0;
  const char* tmp_dir = path_get_temp_dir();
  const char* rv = mem_asprintf("%s%c%s%s%s", tmp_dir, SEPARATOR, prefix, buf, suffix);
  mem_free((void*) tmp_dir);
  return rv;
}
