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
#include "clib/stringbuf.h"

// Helper buffer for start conditions.
StringBuf buf_sb;
void buf_reset()
{
  sbuf_clear(&buf_sb);
}

void buf_cat(const char* text)
{
  sbuf_cat(&buf_sb, text);
}

int current_line;

// This gets called from bison.
void rc_error(const char*s)
{
   err("line %d: %s", current_line - 1, s);
}

///////////////////////////////////////////////////////////////////////////////
// Lexing.
///////////////////////////////////////////////////////////////////////////////
Node* rc_lval;
Array raw_line;
Array line;
Array indents;
int next_token;
int prev_indentation;

static void lex_init()
{
  sbuf_init(&buf_sb, "");
  array_init(&line, Node*);
  array_init(&raw_line, Node*);
  array_init(&indents, int);
  next_token = 0;
  prev_indentation = -1;
}

static Node* lex_read()
{
  int tok = yylex();
  current_line = yylineno;
  if (tok == UNKNOWN){
    err("line %d: invalid characters '%s'", current_line, yytext);
  }
  const char* token_text = yytext;
  if (tok == STRING) token_text = buf_sb.str;
  return node_new_token(tok, mem_strdup(token_text), NULL, yylineno);
}

// Returns non-zero if EOF reached.
static int lex_read_line()
{
  array_clear(&raw_line);

  int counter_p = 0;
  int counter_s = 0;

  for(;;){
    Node* n;
  loop:
    n = lex_read();
    switch(n->type){
      case '(':
        counter_p++;
        break;
      case ')':
        counter_p--;
        break;
      case '[':
        counter_s++;
        break;
      case ']':
        counter_s--;
        break;
      case 0:
        if (raw_line.size == 0) return 1;
        return 0;
      case '\n':
        if (counter_p > 0 or counter_s > 0){
          goto loop;
        }
        return 0;
    }
    array_append(&raw_line, n);
  }
  assert_never();
}

// Bison calls this.
int rc_lex()
{
  if (next_token == line.size){
    // We must read another line and populate the line array.
    array_clear(&line);
    next_token = 0;
    for(;;){

      // Check if end of input.
      if (lex_read_line()){
        if (indents.size > 0){
          array_pop(&indents, int);
          rc_lval = node_new(END);
          return END;
        }
        return 0;
      }

      // Check raw line for any information.
      // Skip empty line.
      if (raw_line.size == 0) continue;
      // Skip line that has only whitespace.
      Node* first = array_get(&raw_line, Node*, 0);
      if (raw_line.size == 1 and first->type == WHITESPACE) continue;

      // Otherwise line is good.

      // Calculate indentation level.
      int indentation;
      if (first->type == WHITESPACE){
        indentation = strlen(first->text);
      } else {
        indentation = 0;
      }

      // Compare indentation level to previous.
      if (indentation == prev_indentation){
        array_append(&line, node_new(SEP));
      }
      if (indentation > prev_indentation){
        array_append(&line, node_new(START));
        array_append(&indents, indentation);
      }
      if (indentation < prev_indentation){
        for(;;){
          int pop_indentation = array_pop(&indents, int);
          if (pop_indentation > indentation) array_append(&line, node_new(END));
          if (pop_indentation == indentation) {
            // Put it back
            array_append(&indents, indentation);
            array_append(&line, node_new(SEP));
            break;
          }
          if (pop_indentation < indentation) {
            err("line %d: invalid indentation level", first->line);
          }
        }
      }
      prev_indentation = indentation;

      // Dump everything else in raw_line that's not whitespace into line.
      for (int i = 0; i < raw_line.size; i++){
        Node* n = array_get(&raw_line, Node*, i);
        if (n->type != WHITESPACE){
          array_append(&line, n);
        }
      }
      break;
    }
  }

  rc_lval = array_get(&line, Node*, next_token);
  next_token++;
  return rc_lval->type;
}

///////////////////////////////////////////////////////////////////////////////
// Parsing.
///////////////////////////////////////////////////////////////////////////////

Node* rc_result;
#include <errno.h>

Node* build_tree(const char* filename)
{
  err_filename = filename;
  FILE* f = fopen(filename, "r");
  if (f == NULL)
    err("cannot open '%s' for reading: %s", filename, strerror(errno));

  lex_init();
  yyin = f;
  rc_parse();
  fclose(f);

  return rc_result;
}

void dump_tokens(const char* filename)
{
  FILE* f = fopen(filename, "r");
  lex_init();
  yyin = f;

  int cur = 2;
  for (int i = 0; i < cur; i++) printf(" ");
  printf("start\n");
  for (int tok = rc_lex(); tok != 0; tok = rc_lex()){
    if (tok == START)
      cur += 2;
    else if (tok == END)
      cur -= 2;
    else {
      for (int i = 0; i < cur; i++) printf(" ");
      printf("token: %4d line: %3d text: %s\n", tok, current_line, rc_lval->text);
    }
  }
  for (int i = 0; i < cur; i++) printf(" ");
  printf("end\n");
  fclose(f);
}
