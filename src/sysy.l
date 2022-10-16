%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>

#include "sysy.tab.hpp"

using namespace std;

%}

/* white space and single line comment */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*$
/* block comment */
BlockComment  "/*"([^*]|\**[^*/])*\**"*/"

/* identifier */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* integer literal */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

/* operators with 2 charactors */
LEqOperator   "<="
GEqOperator   ">="
EqOperator    "=="
NEqOperator   "!="
LAndOperator  "&&"
LOrOperator   "||"

%%

{WhiteSpace}    { }
{LineComment}   { }
{BlockComment}  { }

"int"           { return INT; }
"void"          { return VOID; }
"return"        { return RETURN; }
"const"         { return CONST; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

{LEqOperator}   { yylval.str_val = new string(yytext); return REL_OPERATOR; }
{GEqOperator}   { yylval.str_val = new string(yytext); return REL_OPERATOR; }
{EqOperator}    { yylval.str_val = new string(yytext); return EQ_OPERATOR; }
{NEqOperator}   { yylval.str_val = new string(yytext); return EQ_OPERATOR; }
{LAndOperator}  { yylval.str_val = new string(yytext); return LAND_OPERATOR; }
{LOrOperator}   { yylval.str_val = new string(yytext); return LOR_OPERATOR; }

.               { return yytext[0]; }

%%
