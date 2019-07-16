
#include "interpreter.h"
#include "parser.hpp"

extern int yyflex();
extern char* yytext;
extern Interpreter* interpreter;

std::string& Replace(std::string& out, const std::string& what, const std::string& with, const std::string& instr)
{
	size_t pos = instr.find(what);
	out = instr;

	while (pos != std::string::npos) {
		out.replace(pos, what.length(), with);
		pos = out.find(what, pos + with.length());
	}

	return out;
}

int yylex()
{
	int ret = yyflex();

	switch (ret) {
	case NUMBER:
	case IDENTIFIER:
	case STRING:
		yylval.text_t = new std::string(yytext);

		Replace(*yylval.text_t, "\\n", "\n", yytext);
		interpreter->garbage.push_back(yylval.text_t);

		break;

	default:
		break;
	}

	return ret;
}

void yyerror(const char *s)
{
	std::cout << "* ERROR: ln " << yylloc.first_line << ": " << s << "\n";
}
