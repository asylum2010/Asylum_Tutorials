
%option noyywrap
%option yylineno
%option never-interactive

%{

#include "parser.hpp"
#include "interpreter.h"

#define YY_DECL int yyflex YY_PROTO(( void ))

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 4102)
#	pragma warning(disable : 4273)
#	pragma warning(disable : 4065)
#	pragma warning(disable : 4267)
#	pragma warning(disable : 4244)
#	pragma warning(disable : 4996)
#endif

%}

WHITESPACE		[ \t\n\r]
IDSTART			[a-zA-Z_]
IDCHAR			[a-zA-Z_0-9]
NUMBER			[0-9]|[1-9]([0-9]+)

%x str

%%

{WHITESPACE}+			{ LEXER_OUT("WHITESPACE"); }
"//"(.*)				{ LEXER_OUT("COMMENT"); }

"print"					{ LEXER_OUT("PRINT");									return PRINT; }
"int"					{ LEXER_OUT("INT");										return INT; }
"return"				{ LEXER_OUT("RETURN");									return RETURN; }

"("						{ LEXER_OUT("LRB");										return LRB; }
")"						{ LEXER_OUT("RRB");										return RRB; }
"{"						{ LEXER_OUT("LB");										return LB; }
"}"						{ LEXER_OUT("RB");										return RB; }
"["						{ LEXER_OUT("LSB");										return LSB; }
"]"						{ LEXER_OUT("RSB");										return RSB; }
";"						{ LEXER_OUT("SEMICOLON");								return SEMICOLON; }
"="						{ LEXER_OUT("EQ");										return EQ; }
"\""					{ LEXER_OUT("QUOTE");		BEGIN(str);					return QUOTE; }
","						{ LEXER_OUT("COMMA");									return COMMA; }

{IDSTART}({IDCHAR}*)	{ LEXER_OUT("IDENTIFIER");								return IDENTIFIER; }
{NUMBER}				{ LEXER_OUT("NUMBER");									return NUMBER; }

<str>"\""				{ LEXER_OUT("QUOTE");		BEGIN(INITIAL);				return QUOTE; }
<str>[^\"]+				{ LEXER_OUT("STRING");									return STRING; }

<<EOF>>					{ return 0; }
.						{ LEXER_OUT("ln " << yylineno << ": lexical error");	return 0; }

%%

#ifdef _MSC_VER
#	pragma warning(pop)
#endif
