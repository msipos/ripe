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

const char* util_trim_ends(const char* input)
{
  char* txt = mem_strdup(input);
  txt++;
  txt[strlen(txt)-1] = 0;
  return txt;
}

const char* util_replace(const char* str, const char c, const char* replace)
{
  // Ultra inneficient, but who cares.
  StringBuf sb;
  sbuf_init(&sb, "");
  while(*str != 0){
    if (*str == c){
      sbuf_printf(&sb, "%s", replace);
    } else {
      sbuf_printf(&sb, "%c", *str);
    }
    str++;
  }
  const char* out = mem_strdup(sb.str);
  sbuf_deinit(&sb);
  return out;
}

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

const char* util_signature(const char* ripe_name)
{
  FuncInfo* fi = stran_get_function(ripe_name);

  if (fi == NULL){
    fatal_throw("while writing signature: cannot find static data for '%s'",
                ripe_name);
  }

  StringBuf sb;
  sbuf_init(&sb, "");
  sbuf_printf(&sb, "Value %s(", fi->c_name);

  for (int i = 0; i < fi->num_params; i++){
    sbuf_printf(&sb, "Value %s", util_c_name(fi->param_names[i]));
    if (i != fi->num_params - 1){
      sbuf_printf(&sb, ", ");
    }
  }
  sbuf_printf(&sb, ")");
  const char* rv = mem_strdup(sb.str);
  sbuf_deinit(&sb);
  return rv;
}

bool annot_check_simple(Node* annot_list, int num, const char* args[])
{
  assert(annot_list != NULL);
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
  assert(annot_list != NULL);
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

const char* annot_get_full(Node* annot_list, const char* key, int num)
{
  assert(annot_list->type == ANNOT_LIST); assert(num >= 0);
  
  for (int i = 0; i < node_num_children(annot_list); i++){
    Node* annot = node_get_child(annot_list, i);
    Node* first = node_get_child(annot, 0);
    if (node_num_children(annot) == 1) continue;
    Node* second = node_get_child(annot, 1);
    if (strequal(first->text, key)) {
      if (num == 0) return util_dot_id(second);
      num--;
    }
  }
  return NULL;
}

const char* annot_get(Node* annot_list, const char* key)
{
  return annot_get_full(annot_list, key, 0);
}

typedef struct {
  int type;
  const char* text;
} MapElement;
#define  NM(id)   {id, #id}

MapElement node_map[] = {
  NM(ID),
  NM(INT),
  NM(DOUBLE),
  NM(STRING),
  NM(CHARACTER),
  NM(SYMBOL),
  NM(SEP),
  NM(START),
  NM(END),
  NM(C_CODE),
  {K_NAMESPACE, "namespace"},
  {K_RETURN, "return"},
  {K_TRUE, "true"},
  {K_FALSE, "false"},
  {K_NIL, "nil"},
  {K_EOF, "eof"},
  {K_AND, "and"},
  {K_OR, "or"},
  {K_NOT, "not"},
  {K_BIT_AND, "bit_and"},
  {K_BIT_OR, "bit_or"},
  {K_BIT_XOR, "bit_xor"},
  {K_BIT_NOT, "bit_not"},
  {K_MODULO, "modulo"},
  {K_IF, "if"},
  {K_ELSE, "else"},
  {K_ELIF, "elif"},
  {K_WHILE, "while"},
  {K_BREAK, "break"},
  {K_CONTINUE, "continue"},
  {K_LOOP, "loop"},
  {K_SWITCH, "switch"},
  {K_CASE, "case"},
  {K_IS, "is"},
  {K_TRY, "try"},
  {K_CATCH, "catch"},
  {K_FINALLY, "finally"},
  {K_RAISE, "raise"},
  {K_FOR, "for"},
  {K_IN, "in"},
  {K_PASS, "pass"},
  {K_CLASS, "class"},
  {K_CONSTRUCTOR, "constructor"},
  {K_VIRTUAL_GET, "virtual_get"},
  {K_VIRTUAL_SET, "virtual_set"},
  {K_VAR, "var"},
  {K_ARROW, "=>"},
  {OP_EQUAL, "=="},
  {OP_NOT_EQUAL, "!]"},
  {OP_LTE, "<="},
  {OP_GTE, ">="},
  {0, NULL}
};

const char* util_node_type(int type)
{
  for (int i = 0; node_map[i].type != 0; i++){
    if (node_map[i].type == type) return node_map[i].text;
  }
  if (type < 128) return mem_asprintf("%c", type);
  return mem_asprintf("%d", type);
}

void lang_init()
{
  stran_init();
  wr_init();
  var_init();
  cache_init();
}
