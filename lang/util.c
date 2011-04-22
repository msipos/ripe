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

#include "lang/lang.h"

const char* util_escape(const char* ripe_name)
{
  char tmp[strlen(ripe_name)*2 + 4];
  const char* s = ripe_name;
  strcpy(tmp, "");

  // Not efficient, but who cares...
  while(*s != 0){
    if (*s == '.') {
      strcat(tmp, "_");
    } else if (*s == '?') {
      strcat(tmp, "_Q");
    } else if (*s == '!') {
      strcat(tmp, "_E");
    } else {
      char tmps[2] = {*s, 0};
      strcat(tmp, tmps);
    }
    s++;
  }

  return mem_strdup(tmp);
}

const char* util_c_name(const char* ripe_name)
{
  return mem_asprintf("__%s", util_escape(ripe_name));
}

const char* util_dot_id(Node* expr)
{
  switch(expr->type){
    case ID:
      return mem_strdup(expr->text);
    case EXPR_FIELD:
      {
        Node* parent = node_get_child(expr, 0);
        const char* field = node_get_string(expr, "name");
        const char* s = util_dot_id(parent);
        if (s != NULL) return mem_asprintf("%s.%s", s, field);
        else return NULL;
      }
    default:
      // Anything other than ID or EXPR_FIELD means that it cannot be a
      // static symbol.
      return NULL;
  }
}

bool annot_check_simple(Node* annot_list, int num, const char* args[])
{
  assert(annot_list->type == ANNOT_LIST);

  // Iterate over each annotation
  for (int i = 0; i < node_num_children(annot_list); i++){
    Node* annot = node_get_child(annot_list, i);
    assert(annot->type == ANNOT);
    Node* first = node_get_child(annot, 0);

    // Iterate over each allowed annotation
    bool matched = false;
    for (int j = 0; j < num; j++){
      if (node_num_children(annot) == 1){
        // Plain ID annotation
        if (strequal(args[j], first->text)){
          matched = true;
          break;
        }
      } else {
        // ID = ID annotation
        if (args[j][0] != '=') continue;
        if (strequal(args[j]+1, first->text)){
          matched = true;
          break;
        }
      }
    }

    // This annotation is not allowed
    if (not matched) return false;
  }
  return true;
}

bool annot_check(Node* annot_list, int num, ...)
{
  assert(num > 0);

  va_list ap;
  va_start(ap, num);
  const char* args[num];
  for(int i = 0; i < num; i++){
    args[i] = va_arg(ap, const char*);
  }
  return annot_check_simple(annot_list, num, args);
}

bool annot_has(Node* annot_list, const char* s)
{
  assert(annot_list->type == ANNOT_LIST);

  for (int i = 0; i < node_num_children(annot_list); i++){
    Node* annot = node_get_child(annot_list, i);
    Node* first = node_get_child(annot, 0);
    if (node_num_children(annot) != 1) continue;
    if (strequal(first->text, s)) return true;
  }
  return false;
}
