/* Definitions */

%{
  #include "lang/parser.h"
  // Some docs:
  // For the rules:
  //   - Rule that matches more text wins.
  //   - If 2 rules match same amount of text, the one that's before wins.

  // Buffer for start conditions in the lexer
  // These are implemented in build-tree.c but they are needed here.
  void buf_reset();
  void buf_cat(const char* text);
%}

%option outfile="lang/scanner.c" header-file="lang/scanner.h"
%option noyywrap
%option yylineno
%option nounput
%option noinput
%x COND_STRING

%{
int input_read(char* buf, int max_size); // Used by flex to do reading
%}

%{
#define YY_INPUT(buf,result,max_size) \
   { \
     result = input_read(buf, max_size); \
   }
%}

%%

[0-9]+                          { return INT; }
[0-9]+\.[0-9]*(e|e\-)?[0-9]+    { return DOUBLE; }
\#[^\n]*                        /* Ignore */
[ \t]+                          { return WHITESPACE; }
\n                              { return '\n'; }
\'.\'                           { return CHARACTER; }
\"                              { BEGIN(COND_STRING);
                                  buf_reset(); }
<COND_STRING>[^\\\n\"]+         { buf_cat(yytext); }
<COND_STRING>\\.                { buf_cat(yytext); }
<COND_STRING>\"                 { BEGIN(0);
                                  return STRING; }
\$[^\$]*\$                      { return C_CODE; }
"+"                             { return '+'; }
"-"                             { return '-'; }
"*"                             { return '*'; }
"/"                             { return '/'; }
"="                             { return '='; }
"=="                            { return OP_EQUAL; }
"!="                            { return OP_NOT_EQUAL; }
"("                             { return '('; }
")"                             { return ')'; }
"["                             { return '['; }
"]"                             { return ']'; }
"<"                             { return '<'; }
">"                             { return '>'; }
">="                            { return OP_GTE; }
"<="                            { return OP_LTE; }
":"                             { return ':'; }
"."                             { return '.'; }
","                             { return ','; }
\@                              { return '@'; }
return                          { return K_RETURN; }
namespace                          { return K_NAMESPACE; }
true                            { return K_TRUE; }
false                           { return K_FALSE; }
nil                             { return K_NIL; }
and                             { return K_AND; }
or                              { return K_OR; }
not                             { return K_NOT; }
if                              { return K_IF; }
else                            { return K_ELSE; }
elif                            { return K_ELIF; }
while                           { return K_WHILE; }
break                           { return K_BREAK; }
continue                        { return K_CONTINUE; }
loop                            { return K_LOOP; }
switch                          { return K_SWITCH; }
case                            { return K_CASE; }
is                              { return K_IS; }
eof                             { return K_EOF; }
try                             { return K_TRY; }
catch                           { return K_CATCH; }
finally                         { return K_FINALLY; }
raise                           { return K_RAISE; }
for                             { return K_FOR; }
in                              { return K_IN; }
pass                            { return K_PASS; }
class                           { return K_CLASS; }
constructor                     { return K_CONSTRUCTOR; }
virtual_get                     { return K_VIRTUAL_GET; }
virtual_set                     { return K_VIRTUAL_SET; }
global                          { return K_GLOBAL; }
const                           { return K_CONST; }
[A-Za-z_][A-Za-z0-9_?]*\!?      { return ID; }
\&[A-Za-z_][A-Za-z0-9_?]*       { return SYMBOL; }
.                               { return UNKNOWN; }

%%

/* Functions */
