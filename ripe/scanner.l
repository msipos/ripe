/* Definitions */

%{
  #include "ripe/parser.h"
  
  // Some docs:
  // For the rules:
  //   - Rule that matches more text wins.
  //   - If 2 rules match same amount of text, the one that's before wins.
%}

%option outfile="ripe/scanner.c" header-file="ripe/scanner.h"
%option noyywrap
%option yylineno

%%

[0-9]+                          { return INT; }
[0-9]+\.[0-9]*(e|e\-)?[0-9]+    { return DOUBLE; }
\#[^\n]*                        /* Ignore */
[ \t]+                          { return WHITESPACE; }
\n                              { return '\n'; }
\"[^\"]*\"                      { return STRING; }
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
module                          { return K_MODULE; }
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
eof                             { return K_EOF; }
try                             { return K_TRY; }
catch                           { return K_CATCH; }
for                             { return K_FOR; }
in                              { return K_IN; }
pass                            { return K_PASS; }
class                           { return K_CLASS; }
const                           { return K_CONST; }
[A-Za-z_][A-Za-z0-9_?]*         { return ID; }
.                               { return UNKNOWN; }

%% 

/* Functions */

