
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

std::string ToString(int value)
{
	char rem = 0;
	bool sign = value < 0;

	std::string str("");
	value = abs(value);

	if (value == 0)
		return "0";

	while (value > 0) {
		rem = value % 10;
		value /= 10;

		str.insert(str.begin(), rem + 0x30);
	}

	if (sign)
		str.insert(str.begin(), '-');

	return str;
}

void Interpreter::Const_Add(expression_desc* expr1, expression_desc* expr2, int type)
{
	switch (type) {
	case Type_Integer: {
		int a = atoi(expr1->value.c_str());
		int b = atoi(expr2->value.c_str());
		
		a += b;
		expr1->value = ToString(a);
		} break;

	default:
		INTERP_NERROR(, "Interpreter::Const_Add(): Unknown type", true);
		break;
	}
}

void Interpreter::Const_Sub(expression_desc* expr1, expression_desc* expr2, int type)
{
	switch (type) {
	case Type_Integer: {
		int a = atoi(expr1->value.c_str());
		int b = atoi(expr2->value.c_str());
		
		a -= b;
		expr1->value = ToString(a);
		} break;

	default:
		INTERP_NERROR(, "Interpreter::Const_Sub(): Unknown type", true);
		break;
	}
}

void Interpreter::Const_Mul(expression_desc* expr1, expression_desc* expr2, int type)
{
	switch (type) {
	case Type_Integer: {
		int a = atoi(expr1->value.c_str());
		int b = atoi(expr2->value.c_str());
		
		a *= b;
		expr1->value = ToString(a);
		} break;

	default:
		INTERP_NERROR(, "Interpreter::Const_Mul(): Unknown type", true);
		break;
	}
}

void Interpreter::Const_Div(expression_desc* expr1, expression_desc* expr2, int type)
{
	switch (type) {
	case Type_Integer: {
		int a = atoi(expr1->value.c_str());
		int b = atoi(expr2->value.c_str());
		
		INTERP_NERROR(, "Interpreter::Const_Div(): Division by zero", b == 0);

		a /= b;
		expr1->value = ToString(a);
		} break;

	default:
		INTERP_NERROR(, "Interpreter::Const_Div(): Unknown type", true);
		break;
	}
}

void Interpreter::Const_Mod(expression_desc* expr1, expression_desc* expr2, int type)
{
	switch (type) {
	case Type_Integer: {
		int a = atoi(expr1->value.c_str());
		int b = atoi(expr2->value.c_str());
		
		a %= b;
		expr1->value = ToString(a);
		} break;

	default:
		INTERP_NERROR(, "Interpreter::Const_Mod(): Unknown type", true);
		break;
	}
}

int Interpreter::Sizeof(int type)
{
	switch (type) {
	case Type_Integer:
		return sizeof(int64_t);

	case Type_String:
		return sizeof(std::string*);

	default:
		break;
	}

	return 0;
}

expression_desc* Interpreter::Arithmetic_Expr(expression_desc* expr1, expression_desc* expr2, unsigned char op)
{
	INTERP_ERROR(0, "Interpreter::Arithmetic_Expr(): NULL == expr1", expr1);
	INTERP_ERROR(0, "Interpreter::Arithmetic_Expr(): NULL == expr2", expr2);

	if (expr1->isconst) {
		if (expr2->isconst) {
			// both are oonstant expressions
			expr1->type = std::max(expr1->type, expr2->type);

			switch (op) {
			case OP_ADD_RR:
				Const_Add(expr1, expr2, expr1->type);
				break;

			case OP_SUB_RR:
				Const_Sub(expr1, expr2, expr1->type);
				break;

			case OP_MUL_RR:
				Const_Mul(expr1, expr2, expr1->type);
				break;

			case OP_DIV_RR:
				Const_Div(expr1, expr2, expr1->type);
				break;

			case OP_MOD_RR:
				Const_Mod(expr1, expr2, expr1->type);
				break;

			case OP_AND_RR:
			case OP_OR_RR:
				INTERP_NERROR(0, "Interpreter::Arithmetic_Expr(): Constant logic expressions not yet implemented", true);
				break;

			case OP_SETL_RR:
			case OP_SETLE_RR:
			case OP_SETG_RR:
			case OP_SETGE_RR:
			case OP_SETE_RR:
			case OP_SETNE_RR:
				INTERP_NERROR(0, "Interpreter::Arithmetic_Expr(): Constant relations not yet implemented", true);
				break;

			default:
				break;
			}
			
			expr1->address = UNKNOWN_ADDR;
			expr1->isconst = true;
		} else if (expr2->address == UNKNOWN_ADDR) {
			// expr1 is const, expr2 in EAX
			std::swap(expr1, expr2);
			expr1->bytecode << OP(OP_MOV_RR) << REG(EBX) << REG(EAX);

			switch (expr2->type) {
			case Type_Integer: {
				int64_t a = atoi(expr2->value.c_str());
				expr1->bytecode << OP(OP_MOV_RS) << REG(EAX) << a;
				} break;

			default:
				INTERP_NERROR(0, "Interpreter::Arithmetic_Expr(): Unknown type", true);
				break;
			}

			expr1->bytecode << OP(op) << REG(EAX) << REG(EBX);
			expr1->address = UNKNOWN_ADDR;
			expr1->isconst = false;
		} else {
			// expr1 is const, expr2 is on the stack
			std::swap(expr1, expr2);
			expr1->bytecode << OP(OP_MOV_RM) << REG(EBX) << expr1->address;

			switch (expr2->type) {
			case Type_Integer: {
				int64_t a = atoi(expr2->value.c_str());
				expr1->bytecode << OP(OP_MOV_RS) << REG(EAX) << a;
				} break;

			default:
				INTERP_NERROR(0, "Interpreter::Arithmetic_Expr(): Unknown type", true);
				break;
			}

			expr1->bytecode << OP(op) << REG(EAX) << REG(EBX);
			expr1->address = UNKNOWN_ADDR;
			expr1->isconst = false;
		}
	} else if (expr1->address == UNKNOWN_ADDR) {
		if (expr2->isconst) {
			// expr1 in EAX, expr2 is const
			switch (expr2->type) {
			case Type_Integer: {
				int64_t a = atoi(expr2->value.c_str());
				expr1->bytecode << OP(op - 1) << REG(EAX) << a;
				} break;

			default:
				INTERP_NERROR(0, "Interpreter::Arithmetic_Expr(): Unknown type", true);
				break;
			}

			expr1->address = UNKNOWN_ADDR;
			expr1->isconst = false;
		} else if (expr2->address == UNKNOWN_ADDR) {
			// both in EAX
			expr1->bytecode << OP(OP_PUSH) << REG(EAX) << NIL;
			expr1->bytecode << expr2->bytecode;
			expr1->bytecode << OP(OP_MOV_RR) << REG(EBX) << REG(EAX);
			expr1->bytecode << OP(OP_POP) << REG(EAX) << NIL;

			expr1->bytecode << OP(op) << REG(EAX) << REG(EBX);
			expr1->address = UNKNOWN_ADDR;
			expr1->isconst = false;
		} else {
			// expr1 in EAX, expr2 on the stack
			expr1->bytecode << OP(OP_PUSH) << REG(EAX) << NIL;
			expr1->bytecode << expr2->bytecode;
			expr1->bytecode << OP(OP_MOV_RM) << REG(EBX) << expr2->address;
			expr1->bytecode << OP(OP_POP) << REG(EAX) << NIL;

			expr1->bytecode << OP(op) << REG(EAX) << REG(EBX);
			expr1->address = UNKNOWN_ADDR;
			expr1->isconst = false;
		}
	} else {
		if (expr2->isconst) {
			// expr1 on stack, expr2 const
			expr1->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr1->address;

			switch (expr2->type) {
			case Type_Integer: {
				int64_t a = atoi(expr2->value.c_str());
				expr1->bytecode << OP(op - 1) << REG(EAX) << a;
				} break;

			default:
				INTERP_NERROR(0, "Interpreter::Arithmetic_Expr(): Unknown type", true);
				break;
			}

			expr1->address = UNKNOWN_ADDR;
			expr1->isconst = false;
		} else if (expr2->address == UNKNOWN_ADDR) {
			// expr1 on stack, expr2 in EAX
			expr1->bytecode << expr2->bytecode;
			expr1->bytecode << OP(OP_MOV_RR) << REG(EBX) << REG(EAX);
			expr1->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr1->address;

			expr1->bytecode << OP(op) << REG(EAX) << REG(EBX);
			expr1->address = UNKNOWN_ADDR;
			expr1->isconst = false;
		} else {
			// both on stack
			expr1->bytecode << expr2->bytecode;
			expr1->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr1->address;
			expr1->bytecode << OP(OP_MOV_RM) << REG(EBX) << expr2->address;

			expr1->bytecode << OP(op) << REG(EAX) << REG(EBX);
			expr1->address = UNKNOWN_ADDR;
			expr1->isconst = false;
		}
	}
	
	interpreter->Deallocate(expr2);
	return expr1;
}

expression_desc* Interpreter::Unary_Expr(expression_desc* expr, unary_expr type)
{
	switch (type) {
	case Expr_Inc:
		INTERP_NERROR(0, "In function '" << interpreter->current_func->name <<
			"': '++' requires lvalue", expr->isconst || (expr->address == UNKNOWN_ADDR));

		// "returns with" the new value
		expr->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr->address;
		expr->bytecode << OP(OP_ADD_RS) << REG(EAX) << (int64_t)1;
		expr->bytecode << OP(OP_MOV_MR) << expr->address << REG(EAX);
		
		break;

	case Expr_Dec:
		INTERP_NERROR(0, "In function '" << interpreter->current_func->name <<
			"': '--' requires lvalue", expr->isconst || (expr->address == UNKNOWN_ADDR));

		// "returns with" the new value
		expr->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr->address;
		expr->bytecode << OP(OP_SUB_RS) << REG(EAX) << (int64_t)1;
		expr->bytecode << OP(OP_MOV_MR) << expr->address << REG(EAX);

		break;

	case Expr_Neg:
		if (expr->isconst) {
			int val = atoi(expr->value.c_str());

			if (val < 0)
				expr->value = expr->value.substr(1);
			else
				expr->value = "-" + expr->value;
		} else if (expr->address == UNKNOWN_ADDR) {
			expr->bytecode << OP(OP_NEG) << REG(EAX) << NIL;
		} else {
			expr->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr->address;
			expr->bytecode << OP(OP_NEG) << REG(EAX) << NIL;
			expr->address = UNKNOWN_ADDR;
		}

		break;

	case Expr_Not:
		if (expr->isconst) {
			int val = atoi(expr->value.c_str());

			if (val == 0)
				expr->value = "1";
			else
				expr->value = "0";
		} else if (expr->address == UNKNOWN_ADDR) {
			expr->bytecode << OP(OP_NOT) << REG(EAX) << NIL;
		} else {
			expr->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr->address;
			expr->bytecode << OP(OP_NOT) << REG(EAX) << NIL;
			expr->address = UNKNOWN_ADDR;
		}

		break;

	default:
		break;
	}

	return expr;
}

int yylex()
{
	int ret = yyflex();

	switch (ret) {
	case NUMBER:
	case IDENTIFIER:
	case STRING: {
		yylval.text_t = interpreter->Allocate<std::string>();
		Replace(*yylval.text_t, "\\n", "\n", yytext);
		} break;

	default:
		break;
	}

	return ret;
}

void yyerror(const char *s)
{
	std::cout << "* ERROR: ln " << yylloc.first_line << ": " << s << "\n";
}
