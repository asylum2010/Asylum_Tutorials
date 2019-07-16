
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     QUOTE = 258,
     LRB = 259,
     RRB = 260,
     LB = 261,
     RB = 262,
     LSB = 263,
     RSB = 264,
     SEMICOLON = 265,
     COMMA = 266,
     EQ = 267,
     PEQ = 268,
     MEQ = 269,
     SEQ = 270,
     DEQ = 271,
     OEQ = 272,
     OR = 273,
     AND = 274,
     NOT = 275,
     ISEQU = 276,
     NOTEQU = 277,
     LT = 278,
     LE = 279,
     GT = 280,
     GE = 281,
     PLUS = 282,
     MINUS = 283,
     STAR = 284,
     DIV = 285,
     MOD = 286,
     INC = 287,
     DEC = 288,
     INT = 289,
     VOID = 290,
     PRINT = 291,
     IF = 292,
     ELSE = 293,
     WHILE = 294,
     RETURN = 295,
     NUMBER = 296,
     IDENTIFIER = 297,
     STRING = 298
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 29 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"

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



/* Line 1676 of yacc.c  */
#line 110 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.hpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE yylloc;

