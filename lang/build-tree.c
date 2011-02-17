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
#include <setjmp.h>

jmp_buf jb;

/////////////////////////////////////////////////////////////////////////////
// Input
/////////////////////////////////////////////////////////////////////////////
RipeInput* input;
int input_lineno;
int cur_line1, cur_line2;
int input_colno;

int input_read(char* buf, int max_size)
{
  int num_lines = input->lines.size;
  if (input_lineno > num_lines) return 0;

  char* line;
  array_get2(&(input->lines), &line, input_lineno - 1);

  int left = strlen(line + input_colno - 1);
  if (left <= max_size){
    // Then we can return the whole line
    memcpy(buf, line + input_colno - 1, left);
    input_lineno++;
    input_colno = 1;
    return left;
  } else {
    // We can return up to max_size.
    memcpy(buf, line + input_colno - 1, max_size);
    input_colno += max_size;
    return max_size;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Utilities
///////////////////////////////////////////////////////////////////////////////

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

// This gets called from bison.
void rc_error(const char*s)
{
  build_tree_error = mem_asprintf("%s:%d-%d: %s", input->filename, cur_line1, cur_line2, s);
  longjmp(jb, 2);
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
  if (tok == UNKNOWN){
    build_tree_error = mem_asprintf("%s:%d: invalid characters '%s'",
                                    input->filename, input_lineno, yytext);
    longjmp(jb, 1);
  }
  const char* token_text = yytext;
  if (tok == STRING) token_text = buf_sb.str;
  return node_new_token(tok, mem_strdup(token_text), NULL, input_lineno);
}


// Returns non-zero if EOF reached.
static int lex_read_line()
{
  array_clear(&raw_line);

  int counter_p = 0;
  int counter_s = 0;
  int counter_b = 0;

  cur_line1 = input_lineno;
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
      case '{':
        counter_b++;
        break;
      case '}':
        counter_b--;
        break;
      case 0:
        if (raw_line.size == 0) return 1;
        cur_line2 = input_lineno;
        return 0;
      case '\n':
        if (counter_p > 0 or counter_s > 0 or counter_b > 0){
          goto loop;
        }
        cur_line2 = input_lineno;
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
            build_tree_error = mem_asprintf(
                                "line %d: invalid indentation level",
                                first->line);
            longjmp(jb, 1);
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

/////////////////////////////////////////////////////////////////////////////
// Parsing.
/////////////////////////////////////////////////////////////////////////////

Node* rc_result;
#include <errno.h>
const char* build_tree_error;
Node* build_tree(RipeInput* in)
{
  input = in;
  input_lineno = 1;
  input_colno = 1;
  lex_init();

  if (!setjmp(jb)){
    rc_parse();
  } else {
    // Came back via error
    return NULL;
  }

  return rc_result;
}
