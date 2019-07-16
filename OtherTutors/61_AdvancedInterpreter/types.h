
#ifndef _TYPES_H_
#define _TYPES_H_

enum symbol_type
{
	Type_Unknown = 0,
	Type_Char = 1,
	Type_Short = 2,
	Type_Integer = 3,
	Type_Float = 4,
	Type_String = 5
};

enum statement_type
{
	Type_Expression = 1,
	Type_Conditional = 2,
	Type_Loop = 3,
	Type_Return = 4
};

enum unary_expr
{
	Expr_Inc,
	Expr_Dec,
	Expr_Neg,
	Expr_Not
};

struct expression_desc
{
	ByteStream	bytecode;
	std::string	value;
	int			type;
	int64_t		address;
	bool		isconst;

	expression_desc()
		: address(0), isconst(false)
	{
	}
};

struct declaration_desc
{
	std::string			name;
	ByteStream			bytecode;
	expression_desc*	expr;

	declaration_desc()
		: expr(nullptr)
	{
	}
};

struct statement_desc
{
	ByteStream	bytecode;
	int			type;
	int			scope;

	statement_desc()
		: type(0), scope(1)
	{
	}
};

typedef std::list<expression_desc*> ExprList;
typedef std::list<declaration_desc*> DeclList;

struct symbol_desc
{
	std::string	name;
	ByteStream	bytecode;
	ExprList*	args;

	int64_t		address;
	int			type;
	bool		isfunc;

	symbol_desc()
		: args(nullptr), address(0), type(Type_Unknown), isfunc(false)
	{
	}
};

struct unresolved_reference
{
	symbol_desc* func;	// when function call
	int offset;			// relative offset to jump

	unresolved_reference()
		: func(nullptr), offset(0)
	{
	}
};

typedef std::list<symbol_desc*> SymbolList;
typedef std::list<statement_desc*> StatList;
typedef std::map<std::string, symbol_desc*> SymbolTable;
typedef std::vector<SymbolTable> ScopeTable;

#endif
