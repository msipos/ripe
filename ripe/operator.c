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

typedef struct {
  int type;
  const char* func;
} OperatorTable;

static OperatorTable binary_ot[] =
{
  {'+',          "op_plus"},
  {'-',          "op_minus"},
  {'*',          "op_star"},
  {'/',          "op_slash"},
  {'^',          "op_exp"},
  {K_AND,        "op_and"},
  {K_OR,         "op_or"},
  {K_IN,         "op_in"},
  {K_BIT_AND,    "op_bit_and"},
  {K_BIT_OR,     "op_bit_or"},
  {K_BIT_XOR,    "op_bit_xor"},
  {K_MODULO,     "op_modulo"},
  {OP_EQUAL,     "op_equal"},
  {OP_NOT_EQUAL, "op_not_equal"},
  {'<',          "op_lt"},
  {'>',          "op_gt"},
  {OP_LTE,       "op_lte"},
  {OP_GTE,       "op_gte"},
  {0,            0}
};

bool is_binary_op(Node* node)
{
  int type = node->type;
  if (node_num_children(node) != 2) return false;
  for (int i = 0; binary_ot[i].type != 0; i++){
    if (type == binary_ot[i].type) return true;
  }
  return false;
}

const char* binary_op_map(int type){
  for (int i = 0; binary_ot[i].type != 0; i++){
    if (type == binary_ot[i].type) return binary_ot[i].func;
  }
  assert_never();

  // Crash:
  return NULL;
}

static OperatorTable unary_ot[] =
{
  {'-',          "op_unary_minus"},
  {K_NOT,        "op_unary_not"},
  {K_BIT_NOT,    "op_unary_bit_not"},
  {0,            0}
};

bool is_unary_op(Node* node)
{
  int type = node->type;
  if (node_num_children(node) != 1) return false;
  for (int i = 0; unary_ot[i].type != 0; i++){
    if (type == unary_ot[i].type) return true;
  }
  return false;
}

const char* unary_op_map(int type){
  for (int i = 0; unary_ot[i].type != 0; i++){
    if (type == unary_ot[i].type) return unary_ot[i].func;
  }
  assert_never();

  // Crash:
  return NULL;
}
