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

#include "ripe/ripe.h"

int log_verbosity = 0;
const char* err_filename = NULL;
static int error_line_max;
static int error_line_min;

// Try to populate error_filename and error_line_* with information from this
// node.
static void error_traverse(Node* node)
{
  if (node->line != -1) {
    if (error_line_max == -1) {
      error_line_max = node->line;
      error_line_min = node->line;
    } else {
      if (error_line_max < node->line) error_line_max = node->line;
      if (error_line_min > node->line) error_line_min = node->line;
    }
  }
  for (int i = 0; i < node_num_children(node); i++){
    error_traverse(node_get_child(node, i));
  }
}

// If node != NULL, attempt to query information about the location of the
// error.
void err_node(Node* node, const char* format, ...)
{
  error_line_min = -1;
  error_line_max = -1;
  if (node != NULL) error_traverse(node);
  const char* error_numbers = NULL;
  if (error_line_min != -1){
    if (error_line_min == error_line_max){
      error_numbers = mem_asprintf("%d", error_line_min);
    } else {
      error_numbers = mem_asprintf("%d-%d", error_line_min, error_line_max);
    }
  }

  if (error_numbers != NULL){
    fprintf(stderr, "%s:%s: ", err_filename, error_numbers);
  } else {
    fprintf(stderr, "%s: ", err_filename);
  }

  va_list ap;
  va_start (ap, format);
  vfprintf(stderr, format, ap);
  va_end (ap);
  fprintf(stderr, "\n");
  exit(1);
}

void err(const char* format, ...)
{
  if (err_filename != NULL){
    fprintf(stderr, "%s: ", err_filename);
  }
  fprintf(stderr, "error: ");

  va_list ap;
  va_start (ap, format);
  vfprintf(stderr, format, ap);
  va_end (ap);
  fprintf(stderr, "\n");
  exit(1);
}

void warn(const char* format, ...)
{
  if (err_filename != NULL){
    fprintf(stderr, "%s: ", err_filename);
  }
  fprintf(stderr, "warning: ");

  va_list ap;
  va_start (ap, format);
  vfprintf(stderr, format, ap);
  va_end (ap);
  fprintf(stderr, "\n");
}

void logging(const char* format, ...)
{
  if (not log_verbosity) return;

  fprintf(stderr, "log: ");
  if (err_filename != NULL){
    fprintf(stderr, "%s: ", err_filename);
  }
  va_list ap;
  va_start (ap, format);
  vfprintf(stderr, format, ap);
  va_end (ap);
  fprintf(stderr, "\n");
}
