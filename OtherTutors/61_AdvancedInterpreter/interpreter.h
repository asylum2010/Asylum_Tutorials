
#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <list>
#include <vector>
#include <map>

#include "bytestream.h"
#include "types.h"
#include "variadic_pointer_set.hpp"

// TODO:
// - constant relations/expressions
// - support for atof
// - break with 'isconst' loop

#define LEXER_OUT(x)			//{ std::cout << "* LEXER: " << x << "\n"; }
#define PARSER_OUT(x)			//{ std::cout << "* PARSER: " << x << "\n"; }
#define INTERP_ERROR(r, e, x)	{ if( !(x) ) { std::cout << "* ERROR: " << e << "!\n"; return r; } }
#define INTERP_NERROR(r, e, x)	{ if( x ) { std::cout << "* ERROR: " << e << "!\n"; return r; } }
#define INTERP_WARNING(e)		std::cout << "* WARNING: " << e << "!\n";

#define CODE_SIZE				65536									// marks the exit call, has no other role
#define STACK_SIZE				(32768 * sizeof(int64_t))				// internal stack size
#define ENTRY_SIZE				(sizeof(uint8_t) + 2 * sizeof(int64_t))	// size of one assembly instruction
#define NUM_SPECIAL				2										// count of special statements
#define UNKNOWN_ADDR			INT64_MAX

#define OP(x)					(uint8_t)(x)
#define REG(x)					(int64_t)(x)
#define ADDR(x)					reinterpret_cast<int64_t>(x)
#define NIL						(int64_t)0

#define ARG1_INT(p)				*((int64_t*)(p + sizeof(uint8_t)))
#define ARG2_INT(p)				*((int64_t*)(p + sizeof(uint8_t) + sizeof(int64_t)))
#define ARG1_PTR(p)				*((void**)(p + sizeof(uint8_t)))
#define ARG2_PTR(p)				*((void**)(p + sizeof(uint8_t) + sizeof(int64_t)))
#define STACK_INT(o)			*((int64_t*)(stack + o))

// special opcodes
#define OP_PRINT_R				0x0		// std::cout << reg[arg1];
#define OP_PRINT_M				0x1		// std::cout << *arg1;

// common instructions
#define OP_PUSH					0x20	// push reg[arg1]
#define OP_PUSHADD				0x21	// push reg[arg1] + arg2
#define OP_POP					0x22	// pop reg[arg1]

#define OP_MOV_RS				0x25	// mov reg[arg1], arg2
#define OP_MOV_RR				0x26	// mov reg[arg1], reg[arg2]
#define OP_MOV_RM				0x27	// mov reg[arg1], [EBP - arg2]
#define OP_MOV_MR				0x28	// mov [EBP - arg1], reg[arg2]
#define OP_MOV_MM				0x29	// mov [EBP - arg1], [EBP - arg2]

#define OP_AND_RS				0x30	// and reg[arg1], arg2
#define OP_AND_RR				0x31	// and reg[arg1], reg[arg2]
#define OP_OR_RS				0x32	// or reg[arg1], arg2
#define OP_OR_RR				0x33	// or reg[arg1], reg[arg2]
#define OP_NOT					0x34	// not reg[arg1]

#define OP_SUB_RS				0x35	// sub reg[arg1], arg2
#define OP_SUB_RR				0x36	// sub reg[arg1], reg[arg2]
#define OP_ADD_RS				0x37	// add reg[arg1], arg2
#define OP_ADD_RR				0x38	// add reg[arg1], reg[arg2]
#define OP_MUL_RS				0x39	// mul reg[arg1], arg2
#define OP_MUL_RR				0x3a	// mul reg[arg1], reg[arg2]
#define OP_DIV_RS				0x3b	// div reg[arg1], arg2
#define OP_DIV_RR				0x3c	// div reg[arg1], reg[arg2]
#define OP_MOD_RS				0x3d	// mod reg[arg1], arg2
#define OP_MOD_RR				0x3e	// mod reg[arg1], reg[arg2]
#define OP_NEG					0x3f	// neg reg[arg1]

// these are NOT identical to x86 instructions!!
#define OP_SETL_RS				0x40	// reg[arg1] = (reg[arg1] < arg2)
#define OP_SETL_RR				0x41	// reg[arg1] = (reg[arg1] < reg[arg2])
#define OP_SETLE_RS				0x42	// reg[arg1] = (reg[arg1] <= arg2)
#define OP_SETLE_RR				0x43	// reg[arg1] = (reg[arg1] <= reg[arg2])
#define OP_SETG_RS				0x44	// reg[arg1] = (reg[arg1] > arg2)
#define OP_SETG_RR				0x45	// reg[arg1] = (reg[arg1] > reg[arg2])
#define OP_SETGE_RS				0x46	// reg[arg1] = (reg[arg1] >= arg2)
#define OP_SETGE_RR				0x47	// reg[arg1] = (reg[arg1] >= reg[arg2])
#define OP_SETE_RS				0x48	// reg[arg1] = (reg[arg1] == arg2)
#define OP_SETE_RR				0x49	// reg[arg1] = (reg[arg1] == reg[arg2])
#define OP_SETNE_RS				0x4a	// reg[arg1] = (reg[arg1] != arg2)
#define OP_SETNE_RR				0x4b	// reg[arg1] = (reg[arg1] != reg[arg2])

#define OP_JZ					0x50	// if( reg[arg1] == 0 ) jmp arg2
#define OP_JNZ					0x51	// if( reg[arg1] != 0 ) jmp arg2
#define OP_JMP					0x52	// jmp arg1

// registers
#define EBP						0		// stack base
#define ESP						1		// stack top
#define EAX						2
#define EBX						3
#define ECX						4
#define EDX						5
#define EIP						6		// instruction pointer

class Interpreter
{
	friend int yyparse();
	friend int yylex();

	typedef void (*stm_ptr)(void*, void*);
	static stm_ptr op_special[NUM_SPECIAL];

	// special statements
	static void Print_Reg(void* arg1, void* arg2);
	static void Print_Memory(void* arg1, void* arg2);

private:
	VariadicPointerSet	garbage;
	ScopeTable			scopes;
	ByteStream			program;
	std::string			progname;
	int					entry;
	int64_t				registers[10];
	char*				stack;

	symbol_desc*		current_func;
	size_t				current_scope;
	int					alloc_addr;

	void Cleanup();

	void Const_Add(expression_desc* expr1, expression_desc* expr2, int type);
	void Const_Sub(expression_desc* expr1, expression_desc* expr2, int type);
	void Const_Mul(expression_desc* expr1, expression_desc* expr2, int type);
	void Const_Div(expression_desc* expr1, expression_desc* expr2, int type);
	void Const_Mod(expression_desc* expr1, expression_desc* expr2, int type);

	int Sizeof(int type);
	expression_desc* Arithmetic_Expr(expression_desc* expr1, expression_desc* expr2, unsigned char op);
	expression_desc* Unary_Expr(expression_desc* expr, unary_expr type);

	template <typename value_type>
	value_type* Allocate() {
		value_type* ret = new value_type();
		garbage.Insert<value_type>(ret);

		return ret;
	}

	void Deallocate(void* ptr) {
		garbage.Erase(ptr);
	}
	
public:
	Interpreter();
	~Interpreter();

	bool Compile(const std::string& file);
	bool Link();
	bool Run();

	void Disassemble();
};

#endif

