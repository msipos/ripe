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

// Options:
%defines "ripe/parser.h" // Write out definitions of the terminal symbols
%output  "ripe/parser.c" // Location of parser output file
%name-prefix "rc_" // Define the prefix for all symbols
%error-verbose  // Verbose errors

%{
  #include "ripe/ripe.h"

  Node* operator(Node* a, Node* op, Node* b)
  {
    node_add_child(op, a);
    node_add_child(op, b);
    return op;
  }
%}


%token   ID
%token   INT
%token   DOUBLE
%token   STRING
%token   CHARACTER
// These are actually never encountered in the grammar, but are used by the
// lexer:
%token   COMMENT
%token   WHITESPACE
%token   UNKNOWN
// These are not really tokens but are generated by the lexer (based on
// indentation, etc.)
%token   SEP
%token   START
%token   END
%token   C_CODE
// Keywords are labeled with K_ prefix. All keywords should appear in lex_get2()
// in addition to here.
%token   K_MODULE     "module"
%token   K_RETURN     "return"
%token   K_TRUE       "true"
%token   K_FALSE      "false"
%token   K_NIL        "nil"
%token   K_AND        "and"
%token   K_OR         "or"
%token   K_NOT        "not"
%token   K_IF         "if"
%token   K_ELSE       "else"
%token   K_ELIF       "elif"
%token   K_WHILE      "while"
%token   K_BREAK      "break"
%token   K_CONTINUE   "continue"
%token   K_LOOP       "loop"
%token   K_EOF        "eof"
%token   K_TRY        "try"
%token   K_CATCH      "catch"
%token   K_FOR        "for"
%token   K_IN         "in"
%token   K_PASS       "pass"
%token   K_CLASS      "class"
%token   K_CONST      "const"
%token   OP_EQUAL     "=="
%token   OP_NOT_EQUAL "!="
%token   OP_ASSIGN    ":="
%token   OP_LTE       "<="
%token   OP_GTE       ">="
// Operator-like
%left    "or"
%left    "and"
%left    ':'
%left    '<' "<=" '>' ">="
%left    "==" "!="
%left    '+' '-'
%left    '*' '/'
%left    '['
%left    '.'

%% ////////////////////////////////////////////////////////////// Grammar rules

//         PROGRAM
program:          START toplevel_list END
{
  rc_result = $2;
};

//         TOPLEVEL_LIST
toplevel_list:    toplevel_list SEP toplevel
{
  $$ = $1;
  node_add_child($$, $3);
};
toplevel_list:    toplevel
{
  $$ = node_new(TOPLEVEL_LIST);
  node_add_child($$, $1);
};
toplevel_list:    /* empty */
{
  $$ = node_new(TOPLEVEL_LIST);
};

//         TOPLEVEL
toplevel:           "module" ID START toplevel_list END
{
  $$ = node_new(MODULE);
  node_set_string($$, "name", $2->text);
  node_add_child($$, $4);
};
toplevel:         function
{
  $$ = $1;
};
toplevel:         ID '=' dexpr
{
  $$ = node_new(GLOBAL_VAR);
  node_add_child($$, $1);
  node_add_child($$, $3);
};
toplevel:         C_CODE
{
  $$ = $1;
};
toplevel:         "class" ID START toplevel_list END
{
  $$ = node_new(CLASS);
  node_set_string($$, "name", $2->text);
  node_add_child($$, $4);
};
toplevel:         ID ID
{
  $$ = node_new(TL_VAR);
  node_set_string($$, "annotation", $1->text);
  node_set_string($$, "name", $2->text);
};
toplevel:         ID ID '(' arg_star ')' block
{
  $$ = node_new(ANNOT_FUNCTION);
  node_set_string($$, "annotation", $1->text);
  node_set_string($$, "name", $2->text);
  node_add_child($$, $4);
  node_add_child($$, $6);
};
toplevel:         "const" ID '=' dexpr
{
  $$ = node_new(CONST);
  node_add_child($$, $2);
  node_add_child($$, $4);
};
toplevel:         "const" ID '=' C_CODE
{
  $$ = node_new(CONST);
  node_add_child($$, $2);
  node_add_child($$, $4);
};

function:         ID '(' arg_star ')' block
{
  $$ = node_new(FUNCTION);
  node_set_string($$, "name", $1->text);
  node_add_child($$, $3);
  node_add_child($$, $5);
};

block:            START stmt_list END
{
  $$ = $2;
};

//         STMT_LIST
stmt_list:        stmt_list SEP stmt
{
  $$ = $1;
  if ($3 != NULL) node_add_child($$, $3);
};
stmt_list:        stmt
{
  $$ = node_new(STMT_LIST);
  if ($1 != NULL) node_add_child($$, $1);
};
stmt_list:        /* empty */
{
  $$ = node_new(STMT_LIST);
};

//         STMT
stmt:             "if" expr block
{
  $$ = node_new(STMT_IF);
  node_add_child($$, $2);
  node_add_child($$, $3);
};
stmt:             "else" block
{
  $$ = node_new(STMT_ELSE);
  node_add_child($$, $2);
};
stmt:             "elif" expr block
{
  $$ = node_new(STMT_ELIF);
  node_add_child($$, $2);
  node_add_child($$, $3);
};
stmt:             expr '=' expr
{
  $$ = node_new(STMT_ASSIGN);
  node_add_child($$, $1);
  node_add_child($$, $3);
};
stmt:             expr
{
  $$ = node_new(STMT_EXPR);
  node_add_child($$, $1);
};
stmt:             "return" expr
{
  $$ = node_new(STMT_RETURN);
  node_add_child($$, $2);
};
stmt:             "return"
{
  $$ = node_new(STMT_RETURN);
  node_add_child($$, node_new(K_NIL));
};
stmt:             "try" block SEP "catch" block
{
  $$ = node_new(STMT_TRY);
  node_add_child($$, $2);
  node_add_child($$, $5);
};
stmt:             "while" expr block
{
  $$ = node_new(STMT_WHILE);
  node_add_child($$, $2);
  node_add_child($$, $3);
};
stmt:             "loop" block
{
  $$ = node_new(STMT_LOOP);
  node_add_child($$, $2);
};
stmt:             "for" ID "in" expr block
{
  $$ = node_new(STMT_FOR);
  node_add_child($$, $2);
  node_add_child($$, $4);
  node_add_child($$, $5);
};
stmt:             "break"
{
  $$ = node_new_inherit(STMT_BREAK, $1);
};
stmt:             "continue"
{
  $$ = node_new_inherit(STMT_CONTINUE, $1);
};
stmt:             "pass"
{
  $$ = node_new_inherit(STMT_PASS, $1);
};

//         EXPR
expr:             C_CODE
{
  $$ = $1;
};
expr:             expr '.' ID
{
  $$ = node_new(EXPR_FIELD);
  node_add_child($$, $1);
  node_add_child($$, $3);
};
expr:             expr '.' ID '(' expr_star ')'
{
  $$ = node_new(EXPR_FIELD_CALL);
  node_add_child($$, $1);
  node_add_child($$, $3);
  node_add_child($$, $5);
};
expr:             ID '(' expr_star ')'
{
  $$ = node_new(EXPR_ID_CALL);
  node_add_child($$, $1);
  node_add_child($$, $3);
};
expr:             '(' expr ')'
{
  $$ = $2;
};
expr:             expr '[' expr_plus ']'
{
  $$ = node_new(EXPR_INDEX);
  node_add_child($$, $1);
  node_add_child($$, $3);
};
expr:             '[' expr_star ']'
{
  $$ = node_new(EXPR_ARRAY);
  node_add_child($$, $2);
};
expr:             expr '+' expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr '-' expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr '*' expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr '/' expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr "==" expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr "!=" expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr "and" expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr "or" expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr '<' expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr "<=" expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr '>' expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr ">=" expr
{
  $$ = operator($1, $2, $3);
};
expr:             expr ':' expr
{
  $$ = node_new(EXPR_RANGE_BOUNDED);
  node_add_child($$, $1);
  node_add_child($$, $3);
};
expr:             expr ':'
{
  $$ = node_new(EXPR_RANGE_BOUNDED_LEFT);
  node_add_child($$, $1);
};
expr:             ':' expr
{
  $$ = node_new(EXPR_RANGE_BOUNDED_RIGHT);
  node_add_child($$, $2);
};
expr:             ':'
{
  $$ = node_new(EXPR_RANGE_UNBOUNDED);
};
expr:             '-' expr %prec '.'
{
  $$ = $1;
  node_add_child($$, $2);
};
expr:             "not" expr %prec "and"
{
  $$ = $1;
  node_add_child($$, $2);
};
expr:              '@' ID
{
  $$ = node_new_inherit(EXPR_AT_VAR, $2);
  node_set_string($$, "name", $2->text);
};
expr:              dexpr
{
  $$ = $1;
};
dexpr:             ID
{
  $$ = $1;
};
dexpr:             INT
{
  $$ = $1;
};
dexpr:             DOUBLE
{
   $$ = $1;
};
dexpr:             STRING
{
  $$ = $1;
};
dexpr:             CHARACTER
{
  $$ = $1;
};
dexpr:             "nil"
{
  $$ = $1;
};
dexpr:             "true"
{
  $$ = $1;
};
dexpr:             "false"
{
  $$ = $1;
};
dexpr:             "eof"
{
  $$ = $1;
};

/* Helper rules */
/* expr_star is a list of exprs, possibly empty */
expr_star:        expr_star ',' expr
{
  $$ = $1;
  node_add_child($$, $3);
};
expr_star:        expr
{
  $$ = node_new(EXPR_LIST);
  node_add_child($$, $1);
};
expr_star:        /* empty */
{
  $$ = node_new(EXPR_LIST);
};
/* expr_plus is a list of exprs, at least one */
expr_plus:        expr_plus ',' expr
{
  $$ = $1;
  node_add_child($$, $3);
};
expr_plus:        expr
{
  $$ = node_new(EXPR_LIST);
  node_add_child($$, $1);
};


arg:              '*' ID
{
  $$ = node_new(ARRAY_ARG);
  node_add_child($$, $2);
};
arg:              ID
{
  $$ = $1;
};

arg_star:         arg_star ',' arg
{
  $$ = $1;
  node_add_child($$, $3);
};
arg_star:         arg
{
  $$ = node_new(ARG_LIST);
  node_add_child($$, $1);
};
arg_star:         /* empty */
{
  $$ = node_new(ARG_LIST);
};
