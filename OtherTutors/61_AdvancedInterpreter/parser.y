
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

extern std::string ToString(int value);
extern std::string& Replace(std::string& out, const std::string& what, const std::string& with, const std::string& instr);

%}

%union
{
	std::string*		text_t;
	symbol_desc*		symbol_t;
	SymbolList*			symbollist_t;
	statement_desc*		stat_t;
	StatList*			statlist_t;
	declaration_desc*	decl_t;
	DeclList*			decllist_t;
	expression_desc*	expr_t;
	ExprList*			exprlist_t;
	symbol_type			type_t;
}

// common operators
%token					QUOTE
%token					LRB RRB
%token					LB RB
%token					LSB RSB
%token					SEMICOLON
%token					COMMA

// arithmetic operators
%token					EQ PEQ MEQ SEQ DEQ OEQ
%token					OR AND NOT
%token					ISEQU NOTEQU
%token					LT LE GT GE
%token					PLUS MINUS
%token					STAR DIV MOD
%token					INC DEC

// builtin types
%token					INT
%token					VOID

// keywords
%token					PRINT
%token					IF
%token					ELSE
%token					WHILE
%token					RETURN

// literals
%token<text_t>			NUMBER
%token<text_t>			IDENTIFIER
%token<text_t>			STRING

// typed symbols
%type<text_t>			string
%type<decl_t>			init_declarator
%type<type_t>			typename
%type<symbol_t>			variable
%type<symbol_t>			func_call
%type<symbol_t>			argument
%type<symbol_t>			function
%type<symbol_t>			function_header
%type<symbollist_t>		function_list
%type<symbollist_t>		argument_list

%type<decllist_t>		init_declarator_list
%type<statlist_t>		statement_block
%type<statlist_t>		scope
%type<exprlist_t>		expression_list

%type<stat_t>			print
%type<stat_t>			declaration
%type<stat_t>			conditional
%type<stat_t>			while_loop
%type<stat_t>			control_block
%type<stat_t>			statement

%type<expr_t>			expr
%type<expr_t>			literal
%type<expr_t>			term
%type<expr_t>			unary_expr
%type<expr_t>			multiplicative_expr
%type<expr_t>			additive_expr
%type<expr_t>			or_level_expr
%type<expr_t>			and_level_expr
%type<expr_t>			assignment
%type<expr_t>			relative_expr
%type<expr_t>			compare_expr
%type<expr_t>			lvalue

%%

program: function_list
	{
		PARSER_OUT("program -> function_list");
		interpreter->entry = -1;

		for (auto it = $1->begin(); it != $1->end(); ++it) {
			if ((*it)->name == "main")
				interpreter->entry = interpreter->program.Size();

			(*it)->address = interpreter->program.Size();
			interpreter->program << (*it)->bytecode;
		}

		INTERP_NERROR(0, "Unresolved external 'main'", interpreter->entry == -1);
		interpreter->Deallocate($1);
	}
;

function_list: function
	{
		$$ = interpreter->Allocate<SymbolList>();
		$$->push_back($1);
	}
	| function_list function
	{
		$$ = $1;
		$$->push_back($2);
	}
;

function: function_header scope
	{
		PARSER_OUT("function -> function_header scope");

		// build code
		$$ = $1;

		if ($1->name == "main") {
			$$->bytecode << OP(OP_MOV_RS) << REG(EAX) << (int64_t)CODE_SIZE;
			$$->bytecode << OP(OP_PUSH) << REG(EAX) << NIL;

			$$->bytecode << OP(OP_PUSH) << REG(EBP) << NIL;
			$$->bytecode << OP(OP_MOV_RR) << REG(EBP) << REG(ESP);
		} else {
			$$->bytecode << OP(OP_PUSH) << REG(EBP) << NIL;
			$$->bytecode << OP(OP_MOV_RR) << REG(EBP) << REG(ESP);
		}

		bool found = false;

		for (auto it = $2->begin(); it != $2->end(); ++it) {
			if (*it) {
				if (found) {
					INTERP_WARNING("In function '" << $1->name << "': Unreachable code detected");
					break;
				}

				if ((*it)->type == Type_Return && (*it)->scope == 1)
					found = true;

				$$->bytecode << (*it)->bytecode;
				interpreter->Deallocate(*it);
			}
		}

		INTERP_NERROR(0, "In function '" << $1->name << "': Function must return a value", $1->type != Type_Unknown && !found);

		if ($1->type == Type_Unknown && !found) {
			// void function and no return statement
			$$->bytecode << OP(OP_MOV_RS) << REG(EAX) << (int64_t)0;
			$$->bytecode << OP(OP_MOV_RR) << REG(ESP) << REG(EBP);
			$$->bytecode << OP(OP_POP) << REG(EBP) << NIL;
			$$->bytecode << OP(OP_POP) << REG(EIP) << NIL;
		}

		interpreter->Deallocate($2);
		interpreter->current_func = 0;

		interpreter->alloc_addr = 0;
	}
;

function_header: typename IDENTIFIER LRB RRB
	{
		PARSER_OUT("function_header -> typename IDENTIFIER LRB RRB");

		if (*$2 == "main")
			INTERP_NERROR(0, "Function 'main' must return 'int'", $1 != Type_Integer);

		// functions are in the global scope
		SymbolTable& table0 = interpreter->scopes[0];
		SymbolTable::iterator sym;

		// check for redeclaration
		sym = table0.find(*$2);
		INTERP_NERROR(0, "Conflicting declaration '" << *$2 << "'", sym != table0.end());

		$$ = interpreter->Allocate<symbol_desc>();
		table0[*$2] = $$;

		interpreter->current_func = $$;

		$$->name = *$2;
		$$->type = $1;
		$$->isfunc = true;
		$$->address = UNKNOWN_ADDR;

		interpreter->Deallocate($2);
	}
	| typename IDENTIFIER LRB argument_list RRB
	{
		PARSER_OUT("function_header -> typename IDENTIFIER LRB argument_list RRB");

		if (*$2 == "main")
			INTERP_NERROR(0, "Function 'main' must return 'int'", $1 != Type_Integer);

		// functions are in the global scope
		SymbolTable& table0 = interpreter->scopes[0];
		SymbolTable::iterator sym;

		// check for redeclaration
		sym = table0.find(*$2);
		INTERP_NERROR(0, "Conflicting declaration '" << *$2 << "'", sym != table0.end());

		$$ = interpreter->Allocate<symbol_desc>();
		table0[*$2] = $$;

		interpreter->current_func = $$;

		$$->name = *$2;
		$$->type = $1;
		$$->isfunc = true;
		$$->address = UNKNOWN_ADDR;

		// ehh-ehh
		int scope = (interpreter->current_scope + 1);
		int64_t addr = 16;	// EAX, EBP

		SymbolTable& table = interpreter->scopes[scope];

		// register arguments into the function's scope
		for (auto it = $4->begin(); it != $4->end(); ++it) {
			// this shouldn't occur ever, but...
			sym = table.find((*it)->name);
			INTERP_NERROR(0, "Conflicting declaration '" << (*it)->name << "'", sym != table.end());

			table[(*it)->name] = (*it);

			// update address
			(*it)->address = addr;
			addr += 8;
		}

		interpreter->Deallocate($2);
		interpreter->Deallocate($4);
	}
;

argument_list: argument
	{
		$$ = interpreter->Allocate<SymbolList>();
		$$->push_back($1);
	}
	| argument_list COMMA argument
	{
		$$ = $1;
		$$->push_back($3);
	}
;

argument: typename IDENTIFIER
	{
		$$ = interpreter->Allocate<symbol_desc>();

		$$->name = *$2;
		$$->type = $1;
		$$->address = UNKNOWN_ADDR;

		interpreter->Deallocate($2);
	}
;

statement_block: /* empty */
	{
		PARSER_OUT("statement_block -> epsilon");
		$$ = interpreter->Allocate<StatList>();
	}
	| statement_block statement SEMICOLON
	{
		PARSER_OUT("statement_block -> statement_block statement SEMICOLON");

		$$ = $1;
		$$->push_back($2);
	}
	| statement_block control_block
	{
		PARSER_OUT("statement_block -> statement_block control_block");

		$$ = $1;
		$$->push_back($2);
	}
;

statement: print
	{
		PARSER_OUT("statement -> print");
		$$ = $1;
	}
	| declaration
	{
		PARSER_OUT("statement -> declaration");
		$$ = $1;
	}
	| expr
	{
		// even if it has no sense, like (5 + 3);
		PARSER_OUT("statement -> expr");

		$$ = interpreter->Allocate<statement_desc>();
		$$->bytecode << $1->bytecode;

		interpreter->Deallocate($1);
	}
	| RETURN
	{
		PARSER_OUT("statement -> RETURN");
		symbol_desc* func = interpreter->current_func;

		INTERP_NERROR(0, "In function '" << func->name << "': Invalid return type", func->type != Type_Unknown);

		$$ = interpreter->Allocate<statement_desc>();

		$$->type = Type_Return;
		$$->scope = interpreter->current_scope;

		$$->bytecode << OP(OP_MOV_RS) << REG(EAX) << (int64_t)0;
		$$->bytecode << OP(OP_MOV_RR) << REG(ESP) << REG(EBP);
		$$->bytecode << OP(OP_POP) << REG(EBP) << NIL;
		$$->bytecode << OP(OP_POP) << REG(EIP) << NIL;
	}
	| RETURN expr
	{
		PARSER_OUT("statement -> RETURN expr");
		symbol_desc* func = interpreter->current_func;

		INTERP_NERROR(0, "In function '" << func->name << "': Invalid return type", func->type != $2->type);

		// mov the result into EAX and return
		$$ = interpreter->Allocate<statement_desc>();

		$$->type = Type_Return;
		$$->scope = interpreter->current_scope;

		if ($2->isconst) {
			int64_t val = atoi($2->value.c_str());
			$$->bytecode << OP(OP_MOV_RS) << REG(EAX) << val;
		} else if ($2->address == UNKNOWN_ADDR) {
			$$->bytecode << $2->bytecode;
		} else {
			$$->bytecode << $2->bytecode;
			$$->bytecode << OP(OP_MOV_RM) << REG(EAX) << $2->address;
		}

		$$->bytecode << OP(OP_MOV_RR) << REG(ESP) << REG(EBP);
		$$->bytecode << OP(OP_POP) << REG(EBP) << NIL;
		$$->bytecode << OP(OP_POP) << REG(EIP) << NIL;

		interpreter->Deallocate($2);
	}
;

control_block: conditional
	{
		$$ = $1;
	}
	| while_loop
	{
		$$ = $1;
	}
;

conditional: IF LRB expr RRB scope
	{
		$$ = interpreter->Allocate<statement_desc>();

		if ($3->isconst) {
			int val = atoi($3->value.c_str());

			if (val != 0) {
				for (auto it = $5->begin(); it != $5->end(); ++it) {
					if (*it) {
						$$->bytecode << (*it)->bytecode;
						interpreter->Deallocate(*it);
					}
				}
			}
		} else {
			$$->bytecode << $3->bytecode;

			if ($3->address != UNKNOWN_ADDR)
				$$->bytecode << OP(OP_MOV_RM) << OP(EAX) << $3->address;
	
			int64_t off = 0;
	
			// calculate offset
			for (auto it = $5->begin(); it != $5->end(); ++it) {
				if (*it)
					off += (int64_t)(*it)->bytecode.Size();
			}
	
			$$->bytecode << OP(OP_JZ) << REG(EAX) << off;
	
			// apply true branch
			for (auto it = $5->begin(); it != $5->end(); ++it) {
				if (*it) {
					$$->bytecode << (*it)->bytecode;
					interpreter->Deallocate(*it);
				}
			}
		}

		interpreter->Deallocate($3);
		interpreter->Deallocate($5);
	}
	| IF LRB expr RRB scope ELSE scope
	{
		$$ = interpreter->Allocate<statement_desc>();

		if ($3->isconst) {
			int val = atoi($3->value.c_str());

			if (val != 0) {
				// apply true branch
				for (auto it = $5->begin(); it != $5->end(); ++it) {
					if (*it) {
						$$->bytecode << (*it)->bytecode;
						interpreter->Deallocate(*it);
					}
				}
			} else {
				// apply false branch
				for (auto it = $7->begin(); it != $7->end(); ++it) {
					if (*it) {
						$$->bytecode << (*it)->bytecode;
						interpreter->Deallocate(*it);
					}
				}
			}
		} else {
			$$->bytecode << $3->bytecode;
	
			if ($3->address != UNKNOWN_ADDR)
				$$->bytecode << OP(OP_MOV_RM) << OP(EAX) << $3->address;
	
			int64_t off1 = 0;
			int64_t off2 = 0;
	
			// calculate offset
			for (auto it = $5->begin(); it != $5->end(); ++it) {
				if (*it)
					off1 += (int64_t)(*it)->bytecode.Size();
			}
	
			for (auto it = $7->begin(); it != $7->end(); ++it) {
				if (*it)
					off2 += (int64_t)(*it)->bytecode.Size();
			}
	
			// because the jump below adds some bytes
			$$->bytecode << OP(OP_JZ) << REG(EAX) << (int64_t)(off1 + ENTRY_SIZE);
	
			// apply true branch
			for (auto it = $5->begin(); it != $5->end(); ++it) {
				if (*it) {
					$$->bytecode << (*it)->bytecode;
					interpreter->Deallocate(*it);
				}
			}
	
			$$->bytecode << OP(OP_JMP) << off2 << NIL;
	
			// apply false branch
			for (auto it = $7->begin(); it != $7->end(); ++it) {
				if (*it) {
					$$->bytecode << (*it)->bytecode;
					interpreter->Deallocate(*it);
				}
			}
		}

		interpreter->Deallocate($3);
		interpreter->Deallocate($5);
		interpreter->Deallocate($7);
	}
;

while_loop: WHILE LRB expr RRB scope
	{
		$$ = interpreter->Allocate<statement_desc>();
		$$->bytecode << $3->bytecode;

		INTERP_NERROR(0, "Break statement not yet supported", $3->isconst);

		int64_t off = 0;
		int64_t loop = $3->bytecode.Size();

		if ($3->address != UNKNOWN_ADDR) {
			$$->bytecode << OP(OP_MOV_RM) << OP(EAX) << $3->address;
			loop += ENTRY_SIZE;
		}

		// calculate offset
		for (auto it = $5->begin(); it != $5->end(); ++it) {
			if (*it)
				off += (int64_t)(*it)->bytecode.Size();
		}

		$$->bytecode << OP(OP_JZ) << REG(EAX) << (int64_t)(off + ENTRY_SIZE);

		// jz and jmp
		loop += (off + 2 * ENTRY_SIZE);

		for (auto it = $5->begin(); it != $5->end(); ++it) {
			if (*it) {
				$$->bytecode << (*it)->bytecode;
				interpreter->Deallocate(*it);
			}
		}

		$$->bytecode << OP(OP_JMP) << -loop << NIL;

		interpreter->Deallocate($3);
		interpreter->Deallocate($5);
	}
;

scope: scope_start statement_block RB
	{
		$$ = $2;

		// end of scope, deallocate locals
		int s = interpreter->current_scope;
		int64_t size = 0;

		SymbolTable& table = interpreter->scopes[s];

		for (auto it = table.begin(); it != table.end(); ++it) {
			size += interpreter->Sizeof(it->second->type);
			interpreter->Deallocate(it->second);
		}

		table.clear();

		// function scopes deallocate with the return keyword
		if (interpreter->current_scope > 1 && size > 0) {
			statement_desc* dealloc = interpreter->Allocate<statement_desc>();
			$$->push_back(dealloc);

			dealloc->bytecode << OP(OP_ADD_RS) << REG(ESP) << size;
		}

		--interpreter->current_scope;
	}
;

scope_start: LB
	{
		++interpreter->current_scope;
	}
;

print: PRINT string
	{
		PARSER_OUT("print -> PRINT string");

		$$ = interpreter->Allocate<statement_desc>();
		$$->bytecode << OP(OP_PRINT_M) << ADDR($2) << REG(0);
	}
	| PRINT expr
	{
		PARSER_OUT("print -> PRINT expr");
		$$ = interpreter->Allocate<statement_desc>();

		if ($2->address == UNKNOWN_ADDR) {
			// result of an expression in EAX
			$$->bytecode << $2->bytecode << OP(OP_PRINT_R) << REG(EAX) << REG(0);
		} else {
			// variable on the stack
			$$->bytecode << $2->bytecode;
			$$->bytecode << OP(OP_MOV_RM) << REG(EAX) << $2->address;
			$$->bytecode << OP(OP_PRINT_R) << REG(EAX) << REG(0);
		}

		interpreter->Deallocate($2);
	}
;

declaration: typename init_declarator_list
	{
		PARSER_OUT("declaration -> typename init_declarator_list");

		int scope = interpreter->current_scope;
		int64_t size = 0;

		SymbolTable& table = interpreter->scopes[scope];
		SymbolTable::iterator sym;
		symbol_desc* var;
		expression_desc* expr;

		$$ = interpreter->Allocate<statement_desc>();

		// look for entries in current scope
		for (auto it = $2->begin(); it != $2->end(); ++it) {
			sym = table.find((*it)->name);
			INTERP_NERROR(0, "Conflicting declaration '" << (*it)->name << "'", sym != table.end());

			// save this variable
			var = interpreter->Allocate<symbol_desc>();
			var->type = $1;

			table[(*it)->name] = var;

			// update address
			interpreter->alloc_addr += interpreter->Sizeof($1);
			var->address = -interpreter->alloc_addr;

			// extend size
			size += interpreter->Sizeof($1);
		}

		$$->bytecode << OP(OP_SUB_RS) << REG(ESP) << size;

		for (auto it = $2->begin(); it != $2->end(); ++it) {
			sym = table.find((*it)->name);
			var = sym->second;
			expr = (*it)->expr;

			if (expr) {
				if (expr->isconst) {
					int64_t a = atoi(expr->value.c_str());

					$$->bytecode << OP(OP_MOV_RS) << REG(EAX) << a;
					$$->bytecode << expr->bytecode << OP(OP_MOV_MR) << var->address << REG(EAX);
				} else if (expr->address == UNKNOWN_ADDR) {
					// result of an expression in EAX
					$$->bytecode << expr->bytecode << OP(OP_MOV_MR) << var->address << REG(EAX);
				} else {
					// variable on the stack
					$$->bytecode << expr->bytecode;
					$$->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr->address;
					$$->bytecode << OP(OP_MOV_MR) << var->address << REG(EAX);
				}

				interpreter->Deallocate(expr);
			}

			interpreter->Deallocate(*it);
		}

		interpreter->Deallocate($2);
	}
;

init_declarator_list: init_declarator
	{
		PARSER_OUT("init_declarator_list -> init_declarator");

		$$ = interpreter->Allocate<DeclList>();
		$$->push_back($1);
	}
	| init_declarator_list COMMA init_declarator
	{
		PARSER_OUT("init_declarator_list -> init_declarator_list COMMA init_declarator");

		$$ = $1;
		$$->push_back($3);
	}
;

init_declarator: IDENTIFIER
	{
		PARSER_OUT("init_declarator -> IDENTIFIER");

		$$ = interpreter->Allocate<declaration_desc>();
		$$->name = *$1;

		interpreter->Deallocate($1);
	}
	| IDENTIFIER EQ expr
	{
		PARSER_OUT("init_declarator -> IDENTIFIER EQ expr");

		$$ = interpreter->Allocate<declaration_desc>();
		$$->name = *$1;
		$$->expr = $3;

		interpreter->Deallocate($1);
	}
;

expr: assignment
	{
		PARSER_OUT("expr -> assignment");
		$$ = $1;
	}
;

assignment: or_level_expr
	{
		$$ = $1;
	}
	| lvalue EQ assignment
	{
		PARSER_OUT("assignment -> lvalue = assignment");

		INTERP_ERROR(0, "assignment -> lvalue = assignment: NULL == $1", $1);
		INTERP_ERROR(0, "assignment -> lvalue = assignment: NULL == $3", $3);

		$$ = $1;

		if ($3->isconst) {
			int64_t a = atoi($3->value.c_str());

			$$->bytecode << OP(OP_MOV_RS) << REG(EAX) << a;
			$$->bytecode << $3->bytecode << OP(OP_MOV_MR) << $1->address << REG(EAX);
		} else if ($3->address == UNKNOWN_ADDR) {
			// result of an expression in EAX
			$$->bytecode << $3->bytecode << OP(OP_MOV_MR) << $1->address << REG(EAX);
		} else {
			// variable on the stack
			$$->bytecode << $3->bytecode << OP(OP_MOV_MM) << $1->address << $3->address;
		}

		interpreter->Deallocate($3);
	}
;

or_level_expr: and_level_expr
	{
		$$ = $1;
	}
	| or_level_expr OR and_level_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_OR_RR);
	}
;

and_level_expr: compare_expr
	{
		$$ = $1;
	}
	| and_level_expr AND compare_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_AND_RR);
	}
;

compare_expr: relative_expr
	{
		$$ = $1;
	}
	| compare_expr ISEQU relative_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_SETE_RR);
	}
	| compare_expr NOTEQU relative_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_SETNE_RR);
	}
;

relative_expr: additive_expr
	{
		$$ = $1;
	}
	| relative_expr LT additive_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_SETL_RR);
	}
	| relative_expr LE additive_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_SETLE_RR);
	}
	| relative_expr GT additive_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_SETG_RR);
	}
	| relative_expr GE additive_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_SETGE_RR);
	}
;

additive_expr: multiplicative_expr
	{
		$$ = $1;
	}
	| additive_expr PLUS multiplicative_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_ADD_RR);
	}
	| additive_expr MINUS multiplicative_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_SUB_RR);
	}
;

multiplicative_expr: unary_expr
	{
		$$ = $1;
	}
	| multiplicative_expr STAR unary_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_MUL_RR);
	}
	| multiplicative_expr DIV unary_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_DIV_RR);
	}
	| multiplicative_expr MOD unary_expr
	{
		$$ = interpreter->Arithmetic_Expr($1, $3, OP_MOD_RR);
	}
;

unary_expr: term
	{
		$$ = $1;
	}
	| INC term
	{
		$$ = interpreter->Unary_Expr($2, Expr_Inc);

		if (!$$)
			return 0;
	}
	| DEC term
	{
		$$ = interpreter->Unary_Expr($2, Expr_Dec);

		if (!$$)
			return 0;
	}
	| PLUS term
	{
		$$ = $2;
	}
	| MINUS term
	{
		$$ = interpreter->Unary_Expr($2, Expr_Neg);

		if (!$$)
			return 0;
	}
	| NOT term
	{
		$$ = interpreter->Unary_Expr($2, Expr_Not);

		if (!$$)
			return 0;
	}
;

lvalue: variable
	{
		$$ = interpreter->Allocate<expression_desc>();

		$$->address = $1->address;
		$$->isconst = false;
	}
;

term: LRB expr RRB
	{
		PARSER_OUT("term -> (expr)");
		$$ = $2;
	}
	| literal
	{
		PARSER_OUT("term -> literal");
		$$ = $1;
	}
	| variable
	{
		PARSER_OUT("term -> variable");
		$$ = interpreter->Allocate<expression_desc>();

		$$->address = $1->address;
		$$->isconst = false;
		$$->type = $1->type;
	}
	| func_call
	{
		PARSER_OUT("term -> func_call");
		$$ = interpreter->Allocate<expression_desc>();

		$$->address = UNKNOWN_ADDR;
		$$->isconst = false;
		$$->type = $1->type;

		expression_desc* expr;
		int64_t val;
		int count = 0;

		if ($1->args) {
			for (auto it = $1->args->rbegin(); it != $1->args->rend(); ++it) {
				expr = (*it);

				if (expr->isconst) {
					val = atoi(expr->value.c_str());
					$$->bytecode << OP(OP_MOV_RS) << REG(EAX) << val;
				} else if (expr->address == UNKNOWN_ADDR) {
					$$->bytecode << expr->bytecode;
				} else {
					$$->bytecode << expr->bytecode;
					$$->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr->address;
				}

				$$->bytecode << OP(OP_PUSH) << REG(EAX) << NIL;

				interpreter->Deallocate(expr);
				++count;
			}

			interpreter->Deallocate($1->args);
		}

		unresolved_reference* ref = interpreter->Allocate<unresolved_reference>();
		ref->func = $1;

		$$->bytecode << OP(OP_PUSHADD) << REG(EIP) << (int64_t)(ENTRY_SIZE);
		$$->bytecode << OP(OP_JMP) << UNKNOWN_ADDR << ADDR(ref);

		// clear the stack
		for (int i = 0; i < count; ++i)
			$$->bytecode << OP(OP_POP) << REG(EDX) << NIL;
	}
;

func_call: IDENTIFIER LRB expression_list RRB
	{
		// look for this function in the global scope
		SymbolTable::iterator sym;

		SymbolTable& table = interpreter->scopes[0];
		sym = table.find(*$1);

		INTERP_NERROR(0, "Undeclared function '" << *$1 << "'", sym == table.end());
		INTERP_ERROR(0, "Referenced symbol '" << *$1 << "' is not a function", sym->second->isfunc);

		$$ = sym->second;
		$$->args = $3;

		interpreter->Deallocate($1);
	}
	| IDENTIFIER LRB RRB
	{
		// look for this function in the global scope
		SymbolTable::iterator sym;

		SymbolTable& table = interpreter->scopes[0];
		sym = table.find(*$1);

		INTERP_NERROR(0, "Undeclared function '" << *$1 << "'", sym == table.end());
		INTERP_ERROR(0, "Referenced symbol '" << *$1 << "' is not a function", sym->second->isfunc);

		$$ = sym->second;
		$$->args = 0;

		interpreter->Deallocate($1);
	}
;

expression_list: expr
	{
		$$ = interpreter->Allocate<ExprList>();
		$$->push_back($1);
	}
	| expression_list COMMA expr
	{
		$$ = $1;
		$$->push_back($3);
	}
;

literal: NUMBER
	{
		PARSER_OUT("literal -> NUMBER");
		$$ = interpreter->Allocate<expression_desc>();

		$$->type = Type_Integer;
		$$->value = *$1;
		$$->address = UNKNOWN_ADDR;
		$$->isconst = true;

		interpreter->Deallocate($1);
	}
;

variable: IDENTIFIER
	{
		PARSER_OUT("variable -> IDENTIFIER");

		// look for this variable in the symbol table
		bool found = false;
		SymbolTable::iterator sym;

		for (int s = interpreter->current_scope; s >= 0; --s) {
			SymbolTable& table = interpreter->scopes[s];
			sym = table.find(*$1);

			if (sym != table.end()) {
				found = true;
				break;
			}
		}

		INTERP_ERROR(0, "Undeclared identifier '" << *$1 << "'", found);
		$$ = sym->second;

		interpreter->Deallocate($1);
	}
;

typename: INT
	{
		PARSER_OUT("typename -> INT");
		$$ = Type_Integer;
	}
	| VOID
	{
		PARSER_OUT("typename -> VOID");
		$$ = Type_Unknown;
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
