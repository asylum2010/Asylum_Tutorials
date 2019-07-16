
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

WHITESPACE		[ \t]
IDSTART			[a-zA-Z_]
IDCHAR			[a-zA-Z_0-9]
NUMBER			[0-9]|[1-9]([0-9]+)

%x lex_str

%%

"\r\n"					{ ++yylloc.first_line; } // win
"\n"					{ ++yylloc.first_line; } // unix

{WHITESPACE}+			{ LEXER_OUT("WHITESPACE"); }
"//"(.*)				{ LEXER_OUT("COMMENT"); }

"int"					{ LEXER_OUT("INT");							return INT; }
"void"					{ LEXER_OUT("VOID");						return VOID; }

"print"					{ LEXER_OUT("PRINT");						return PRINT; }
"if"					{ LEXER_OUT("IF");							return IF; }
"else"					{ LEXER_OUT("ELSE");						return ELSE; }
"while"					{ LEXER_OUT("WHILE");						return WHILE; }
"return"				{ LEXER_OUT("RETURN");						return RETURN; }

"="						{ LEXER_OUT("EQ");							return EQ; }
"+="					{ LEXER_OUT("PEQ");							return PEQ; }
"-="					{ LEXER_OUT("MEQ");							return MEQ; }
"*="					{ LEXER_OUT("SEQ");							return SEQ; }
"/="					{ LEXER_OUT("DEQ");							return DEQ; }
"%="					{ LEXER_OUT("OEQ");							return OEQ; }

"||"					{ LEXER_OUT("OR");							return OR; }
"&&"					{ LEXER_OUT("AND");							return AND; }

"=="					{ LEXER_OUT("ISEQU");						return ISEQU; }
"!="					{ LEXER_OUT("NOTEQU");						return NOTEQU; }
"!"						{ LEXER_OUT("NOT");							return NOT; }

"<"						{ LEXER_OUT("LT");							return LT; }
"<="					{ LEXER_OUT("LE");							return LE; }
">"						{ LEXER_OUT("GT");							return GT; }
">="					{ LEXER_OUT("GE");							return GE; }

"+"						{ LEXER_OUT("PLUS");						return PLUS; }
"-"						{ LEXER_OUT("MINUS");						return MINUS; }
"*"						{ LEXER_OUT("STAR");						return STAR; }
"/"						{ LEXER_OUT("DIV");							return DIV; }
"%"						{ LEXER_OUT("MOD");							return MOD; }

"++"					{ LEXER_OUT("INC");							return INC; }
"--"					{ LEXER_OUT("DEC");							return DEC; }

"("						{ LEXER_OUT("LRB");							return LRB; }
")"						{ LEXER_OUT("RRB");							return RRB; }
"{"						{ LEXER_OUT("LB");							return LB; }
"}"						{ LEXER_OUT("RB");							return RB; }
"["						{ LEXER_OUT("LSB");							return LSB; }
"]"						{ LEXER_OUT("RSB");							return RSB; }
";"						{ LEXER_OUT("SEMICOLON");					return SEMICOLON; }
"\""					{ LEXER_OUT("QUOTE");		BEGIN(lex_str);	return QUOTE; }
","						{ LEXER_OUT("COMMA");						return COMMA; }

{IDSTART}({IDCHAR}*)	{ LEXER_OUT("IDENTIFIER");					return IDENTIFIER; }
{NUMBER}				{ LEXER_OUT("NUMBER");						return NUMBER; }

<lex_str>"\""			{ LEXER_OUT("QUOTE");		BEGIN(INITIAL);	return QUOTE; }
<lex_str>[^\"]+			{ LEXER_OUT("STRING");						return STRING; }

<<EOF>>					{ return 0; }
.						{ LEXER_OUT("ln " << yylineno << ": lexical error");  return 0; }

%%

#ifdef _MSC_VER
#    pragma warning(pop)
#endif
