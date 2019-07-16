%locations

%{

#include "interpreter.h"

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 4102)
#	pragma warning(disable : 4273)
#	pragma warning(disable : 4065)
#	pragma warning(disable : 4267)
#	pragma warning(disable : 4244)
#	pragma warning(disable : 4996)
#endif

int yylex();
void yyerror(const char *s);

Interpreter* interpreter = nullptr;

%}

%union
{
	std::string* text_t;
}

%token				QUOTE
%token				LRB
%token				RRB
%token				LB
%token				RB
%token				LSB
%token				RSB
%token				SEMICOLON
%token				EQ
%token				COMMA
%token				PRINT
%token				INT
%token				RETURN

%token<text_t>		NUMBER
%token<text_t>		IDENTIFIER
%token<text_t>		STRING

%type<text_t>		string

%%

program: function
	{
		PARSER_OUT("program -> function");
	}
;

function: function_header LB function_body RB
	{
		PARSER_OUT("function -> function_header LB function_body RB");
	}
;

function_header: typename IDENTIFIER LRB argument_list RRB
	{
		PARSER_OUT("function_header -> typename IDENTIFIER LRB argument_list RRB");
	}
;

function_body: statement_block
	{
		PARSER_OUT("function_body -> statement_block");
	}
;

argument_list: /* empty */
;

typename: INT
	{
		PARSER_OUT("typename -> INT");
	}
;

statement_block: statement SEMICOLON
	{
		PARSER_OUT("statement_block -> statement SEMICOLON");
	}
	| statement_block statement SEMICOLON
	{
		PARSER_OUT("statement_block -> statement_block statement SEMICOLON ");
	}
;

statement: print
	{
		PARSER_OUT("statement -> print");
	}
	| return_statement
	{
		PARSER_OUT("statement -> return_statement");
	}
;

return_statement: RETURN NUMBER
	{
		PARSER_OUT("statement -> RETURN NUMBER");
	}
;

print: PRINT string
	{
		PARSER_OUT("print -> PRINT string");
		interpreter->AddCodeEntry(OP_PRINT, interpreter->Allocate<std::string>($2), 0);
	}
;

string: QUOTE STRING QUOTE
	{
		PARSER_OUT("string -> QUOTE STRING QUOTE");
		$$ = $2;
	}
;

%%

#ifdef _MSC_VER
#	pragma warning(pop)
#endif
