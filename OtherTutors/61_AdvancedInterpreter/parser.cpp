
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 4 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"


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



/* Line 189 of yacc.c  */
#line 98 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


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

/* Line 214 of yacc.c  */
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



/* Line 214 of yacc.c  */
#line 192 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.cpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

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


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 217 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  8
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   132

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  44
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  35
/* YYNRULES -- Number of rules.  */
#define YYNRULES  74
/* YYNRULES -- Number of states.  */
#define YYNSTATES  125

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   298

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    13,    18,    24,    26,
      30,    33,    34,    38,    41,    43,    45,    47,    49,    52,
      54,    56,    62,    70,    76,    80,    82,    85,    88,    91,
      93,    97,    99,   103,   105,   107,   111,   113,   117,   119,
     123,   125,   129,   133,   135,   139,   143,   147,   151,   153,
     157,   161,   163,   167,   171,   175,   177,   180,   183,   186,
     189,   192,   194,   198,   200,   202,   204,   209,   213,   215,
     219,   221,   223,   225,   227
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      45,     0,    -1,    46,    -1,    47,    -1,    46,    47,    -1,
      48,    56,    -1,    77,    42,     4,     5,    -1,    77,    42,
       4,    49,     5,    -1,    50,    -1,    49,    11,    50,    -1,
      77,    42,    -1,    -1,    51,    52,    10,    -1,    51,    53,
      -1,    58,    -1,    59,    -1,    62,    -1,    40,    -1,    40,
      62,    -1,    54,    -1,    55,    -1,    37,     4,    62,     5,
      56,    -1,    37,     4,    62,     5,    56,    38,    56,    -1,
      39,     4,    62,     5,    56,    -1,    57,    51,     7,    -1,
       6,    -1,    36,    78,    -1,    36,    62,    -1,    77,    60,
      -1,    61,    -1,    60,    11,    61,    -1,    42,    -1,    42,
      12,    62,    -1,    63,    -1,    64,    -1,    71,    12,    63,
      -1,    65,    -1,    64,    18,    65,    -1,    66,    -1,    65,
      19,    66,    -1,    67,    -1,    66,    21,    67,    -1,    66,
      22,    67,    -1,    68,    -1,    67,    23,    68,    -1,    67,
      24,    68,    -1,    67,    25,    68,    -1,    67,    26,    68,
      -1,    69,    -1,    68,    27,    69,    -1,    68,    28,    69,
      -1,    70,    -1,    69,    29,    70,    -1,    69,    30,    70,
      -1,    69,    31,    70,    -1,    72,    -1,    32,    72,    -1,
      33,    72,    -1,    27,    72,    -1,    28,    72,    -1,    20,
      72,    -1,    76,    -1,     4,    62,     5,    -1,    75,    -1,
      76,    -1,    73,    -1,    42,     4,    74,     5,    -1,    42,
       4,     5,    -1,    62,    -1,    74,    11,    62,    -1,    41,
      -1,    42,    -1,    34,    -1,    35,    -1,     3,    43,     3,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   114,   114,   132,   137,   144,   196,   223,   272,   277,
     284,   297,   301,   308,   317,   322,   327,   337,   354,   385,
     389,   395,   438,   510,   550,   579,   585,   592,   611,   677,
     684,   693,   702,   714,   721,   725,   751,   755,   761,   765,
     771,   775,   779,   785,   789,   793,   797,   801,   807,   811,
     815,   821,   825,   829,   833,   839,   843,   850,   857,   861,
     868,   877,   886,   891,   896,   905,   953,   969,   987,   992,
     999,  1013,  1038,  1043,  1050
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "QUOTE", "LRB", "RRB", "LB", "RB", "LSB",
  "RSB", "SEMICOLON", "COMMA", "EQ", "PEQ", "MEQ", "SEQ", "DEQ", "OEQ",
  "OR", "AND", "NOT", "ISEQU", "NOTEQU", "LT", "LE", "GT", "GE", "PLUS",
  "MINUS", "STAR", "DIV", "MOD", "INC", "DEC", "INT", "VOID", "PRINT",
  "IF", "ELSE", "WHILE", "RETURN", "NUMBER", "IDENTIFIER", "STRING",
  "$accept", "program", "function_list", "function", "function_header",
  "argument_list", "argument", "statement_block", "statement",
  "control_block", "conditional", "while_loop", "scope", "scope_start",
  "print", "declaration", "init_declarator_list", "init_declarator",
  "expr", "assignment", "or_level_expr", "and_level_expr", "compare_expr",
  "relative_expr", "additive_expr", "multiplicative_expr", "unary_expr",
  "lvalue", "term", "func_call", "expression_list", "literal", "variable",
  "typename", "string", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    44,    45,    46,    46,    47,    48,    48,    49,    49,
      50,    51,    51,    51,    52,    52,    52,    52,    52,    53,
      53,    54,    54,    55,    56,    57,    58,    58,    59,    60,
      60,    61,    61,    62,    63,    63,    64,    64,    65,    65,
      66,    66,    66,    67,    67,    67,    67,    67,    68,    68,
      68,    69,    69,    69,    69,    70,    70,    70,    70,    70,
      70,    71,    72,    72,    72,    72,    73,    73,    74,    74,
      75,    76,    77,    77,    78
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     2,     2,     4,     5,     1,     3,
       2,     0,     3,     2,     1,     1,     1,     1,     2,     1,
       1,     5,     7,     5,     3,     1,     2,     2,     2,     1,
       3,     1,     3,     1,     1,     3,     1,     3,     1,     3,
       1,     3,     3,     1,     3,     3,     3,     3,     1,     3,
       3,     1,     3,     3,     3,     1,     2,     2,     2,     2,
       2,     1,     3,     1,     1,     1,     4,     3,     1,     3,
       1,     1,     1,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    72,    73,     0,     2,     3,     0,     0,     1,     4,
      25,     5,    11,     0,     0,     0,     0,    24,     0,     0,
       0,     0,     0,     0,     0,     0,    17,    70,    71,     0,
      13,    19,    20,    14,    15,    16,    33,    34,    36,    38,
      40,    43,    48,    51,     0,    55,    65,    63,    64,     0,
       6,     0,     8,     0,     0,    60,    64,    58,    59,    56,
      57,     0,    27,    26,     0,     0,    18,     0,    12,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    31,    28,    29,     7,     0,    10,    62,
       0,     0,     0,    67,    68,     0,    37,    39,    41,    42,
      44,    45,    46,    47,    49,    50,    52,    53,    54,    35,
       0,     0,     9,    74,     0,     0,    66,     0,    32,    30,
      21,    23,    69,     0,    22
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     3,     4,     5,     6,    51,    52,    14,    29,    30,
      31,    32,    11,    12,    33,    34,    84,    85,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      95,    47,    56,     7,    63
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -101
static const yytype_int8 yypact[] =
{
     -17,  -101,  -101,    20,   -17,  -101,    -5,   -20,  -101,  -101,
    -101,  -101,  -101,    26,    45,    24,    84,  -101,     1,     1,
       1,     1,     1,     4,    31,    52,    84,  -101,    79,    34,
    -101,  -101,  -101,  -101,  -101,  -101,  -101,    10,    74,    19,
      66,    39,    40,  -101,    89,  -101,  -101,  -101,    90,    63,
    -101,    14,  -101,    64,   103,  -101,  -101,  -101,  -101,  -101,
    -101,    67,  -101,  -101,    84,    84,  -101,     6,  -101,    84,
      84,    84,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    97,   102,  -101,  -101,   -17,  -101,  -101,
     111,   110,   113,  -101,  -101,    16,    74,    19,    66,    66,
      39,    39,    39,    39,    40,    40,  -101,  -101,  -101,  -101,
      84,    63,  -101,  -101,    -5,    -5,  -101,    84,  -101,  -101,
      85,  -101,  -101,    -5,  -101
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -101,  -101,  -101,   120,  -101,  -101,    41,  -101,  -101,  -101,
    -101,  -101,  -100,  -101,  -101,  -101,  -101,    21,   -10,    47,
    -101,    58,    60,     3,    46,    17,    18,  -101,    42,  -101,
    -101,  -101,   -14,   -11,  -101
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -62
static const yytype_int8 yytable[] =
{
      48,    10,    48,    49,    53,    16,    54,    61,    16,    48,
      16,    93,    48,    62,   120,   121,    66,     1,     2,    86,
       8,   116,    13,   124,    18,    87,    18,   117,    69,    50,
      15,    19,    20,    19,    20,    64,    21,    22,    21,    22,
      71,    72,    27,    28,    68,    27,    28,    27,    28,    16,
      48,    48,    17,    48,    91,    92,    65,    94,     1,     2,
      55,    57,    58,    59,    60,    18,    77,    78,    48,    79,
      80,    81,    19,    20,    98,    99,    53,    21,    22,     1,
       2,    23,    24,    67,    25,    26,    27,    28,    16,    73,
      74,    75,    76,    70,   104,   105,    48,   106,   107,   108,
     118,    82,   -61,    48,    18,    83,    88,   122,    89,   110,
      90,    19,    20,   111,   113,   114,    21,    22,   115,   100,
     101,   102,   103,   123,     9,    27,    28,    96,   112,   109,
      97,     0,   119
};

static const yytype_int8 yycheck[] =
{
      14,     6,    16,    14,    15,     4,    16,     3,     4,    23,
       4,     5,    26,    23,   114,   115,    26,    34,    35,     5,
       0,     5,    42,   123,    20,    11,    20,    11,    18,     5,
       4,    27,    28,    27,    28,     4,    32,    33,    32,    33,
      21,    22,    41,    42,    10,    41,    42,    41,    42,     4,
      64,    65,     7,    67,    64,    65,     4,    67,    34,    35,
      18,    19,    20,    21,    22,    20,    27,    28,    82,    29,
      30,    31,    27,    28,    71,    72,    87,    32,    33,    34,
      35,    36,    37,     4,    39,    40,    41,    42,     4,    23,
      24,    25,    26,    19,    77,    78,   110,    79,    80,    81,
     110,    12,    12,   117,    20,    42,    42,   117,     5,    12,
      43,    27,    28,    11,     3,     5,    32,    33,     5,    73,
      74,    75,    76,    38,     4,    41,    42,    69,    87,    82,
      70,    -1,   111
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    34,    35,    45,    46,    47,    48,    77,     0,    47,
       6,    56,    57,    42,    51,     4,     4,     7,    20,    27,
      28,    32,    33,    36,    37,    39,    40,    41,    42,    52,
      53,    54,    55,    58,    59,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    75,    76,    77,
       5,    49,    50,    77,    62,    72,    76,    72,    72,    72,
      72,     3,    62,    78,     4,     4,    62,     4,    10,    18,
      19,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    12,    42,    60,    61,     5,    11,    42,     5,
      43,    62,    62,     5,    62,    74,    65,    66,    67,    67,
      68,    68,    68,    68,    69,    69,    70,    70,    70,    63,
      12,    11,    50,     3,     5,     5,     5,    11,    62,    61,
      56,    56,    62,    38,    56
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[2];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 115 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("program -> function_list");
		interpreter->entry = -1;

		for (auto it = (yyvsp[(1) - (1)].symbollist_t)->begin(); it != (yyvsp[(1) - (1)].symbollist_t)->end(); ++it) {
			if ((*it)->name == "main")
				interpreter->entry = interpreter->program.Size();

			(*it)->address = interpreter->program.Size();
			interpreter->program << (*it)->bytecode;
		}

		INTERP_NERROR(0, "Unresolved external 'main'", interpreter->entry == -1);
		interpreter->Deallocate((yyvsp[(1) - (1)].symbollist_t));
	;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 133 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.symbollist_t) = interpreter->Allocate<SymbolList>();
		(yyval.symbollist_t)->push_back((yyvsp[(1) - (1)].symbol_t));
	;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 138 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.symbollist_t) = (yyvsp[(1) - (2)].symbollist_t);
		(yyval.symbollist_t)->push_back((yyvsp[(2) - (2)].symbol_t));
	;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 145 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("function -> function_header scope");

		// build code
		(yyval.symbol_t) = (yyvsp[(1) - (2)].symbol_t);

		if ((yyvsp[(1) - (2)].symbol_t)->name == "main") {
			(yyval.symbol_t)->bytecode << OP(OP_MOV_RS) << REG(EAX) << (int64_t)CODE_SIZE;
			(yyval.symbol_t)->bytecode << OP(OP_PUSH) << REG(EAX) << NIL;

			(yyval.symbol_t)->bytecode << OP(OP_PUSH) << REG(EBP) << NIL;
			(yyval.symbol_t)->bytecode << OP(OP_MOV_RR) << REG(EBP) << REG(ESP);
		} else {
			(yyval.symbol_t)->bytecode << OP(OP_PUSH) << REG(EBP) << NIL;
			(yyval.symbol_t)->bytecode << OP(OP_MOV_RR) << REG(EBP) << REG(ESP);
		}

		bool found = false;

		for (auto it = (yyvsp[(2) - (2)].statlist_t)->begin(); it != (yyvsp[(2) - (2)].statlist_t)->end(); ++it) {
			if (*it) {
				if (found) {
					INTERP_WARNING("In function '" << (yyvsp[(1) - (2)].symbol_t)->name << "': Unreachable code detected");
					break;
				}

				if ((*it)->type == Type_Return && (*it)->scope == 1)
					found = true;

				(yyval.symbol_t)->bytecode << (*it)->bytecode;
				interpreter->Deallocate(*it);
			}
		}

		INTERP_NERROR(0, "In function '" << (yyvsp[(1) - (2)].symbol_t)->name << "': Function must return a value", (yyvsp[(1) - (2)].symbol_t)->type != Type_Unknown && !found);

		if ((yyvsp[(1) - (2)].symbol_t)->type == Type_Unknown && !found) {
			// void function and no return statement
			(yyval.symbol_t)->bytecode << OP(OP_MOV_RS) << REG(EAX) << (int64_t)0;
			(yyval.symbol_t)->bytecode << OP(OP_MOV_RR) << REG(ESP) << REG(EBP);
			(yyval.symbol_t)->bytecode << OP(OP_POP) << REG(EBP) << NIL;
			(yyval.symbol_t)->bytecode << OP(OP_POP) << REG(EIP) << NIL;
		}

		interpreter->Deallocate((yyvsp[(2) - (2)].statlist_t));
		interpreter->current_func = 0;

		interpreter->alloc_addr = 0;
	;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 197 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("function_header -> typename IDENTIFIER LRB RRB");

		if (*(yyvsp[(2) - (4)].text_t) == "main")
			INTERP_NERROR(0, "Function 'main' must return 'int'", (yyvsp[(1) - (4)].type_t) != Type_Integer);

		// functions are in the global scope
		SymbolTable& table0 = interpreter->scopes[0];
		SymbolTable::iterator sym;

		// check for redeclaration
		sym = table0.find(*(yyvsp[(2) - (4)].text_t));
		INTERP_NERROR(0, "Conflicting declaration '" << *(yyvsp[(2) - (4)].text_t) << "'", sym != table0.end());

		(yyval.symbol_t) = interpreter->Allocate<symbol_desc>();
		table0[*(yyvsp[(2) - (4)].text_t)] = (yyval.symbol_t);

		interpreter->current_func = (yyval.symbol_t);

		(yyval.symbol_t)->name = *(yyvsp[(2) - (4)].text_t);
		(yyval.symbol_t)->type = (yyvsp[(1) - (4)].type_t);
		(yyval.symbol_t)->isfunc = true;
		(yyval.symbol_t)->address = UNKNOWN_ADDR;

		interpreter->Deallocate((yyvsp[(2) - (4)].text_t));
	;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 224 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("function_header -> typename IDENTIFIER LRB argument_list RRB");

		if (*(yyvsp[(2) - (5)].text_t) == "main")
			INTERP_NERROR(0, "Function 'main' must return 'int'", (yyvsp[(1) - (5)].type_t) != Type_Integer);

		// functions are in the global scope
		SymbolTable& table0 = interpreter->scopes[0];
		SymbolTable::iterator sym;

		// check for redeclaration
		sym = table0.find(*(yyvsp[(2) - (5)].text_t));
		INTERP_NERROR(0, "Conflicting declaration '" << *(yyvsp[(2) - (5)].text_t) << "'", sym != table0.end());

		(yyval.symbol_t) = interpreter->Allocate<symbol_desc>();
		table0[*(yyvsp[(2) - (5)].text_t)] = (yyval.symbol_t);

		interpreter->current_func = (yyval.symbol_t);

		(yyval.symbol_t)->name = *(yyvsp[(2) - (5)].text_t);
		(yyval.symbol_t)->type = (yyvsp[(1) - (5)].type_t);
		(yyval.symbol_t)->isfunc = true;
		(yyval.symbol_t)->address = UNKNOWN_ADDR;

		// ehh-ehh
		int scope = (interpreter->current_scope + 1);
		int64_t addr = 16;	// EAX, EBP

		SymbolTable& table = interpreter->scopes[scope];

		// register arguments into the function's scope
		for (auto it = (yyvsp[(4) - (5)].symbollist_t)->begin(); it != (yyvsp[(4) - (5)].symbollist_t)->end(); ++it) {
			// this shouldn't occur ever, but...
			sym = table.find((*it)->name);
			INTERP_NERROR(0, "Conflicting declaration '" << (*it)->name << "'", sym != table.end());

			table[(*it)->name] = (*it);

			// update address
			(*it)->address = addr;
			addr += 8;
		}

		interpreter->Deallocate((yyvsp[(2) - (5)].text_t));
		interpreter->Deallocate((yyvsp[(4) - (5)].symbollist_t));
	;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 273 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.symbollist_t) = interpreter->Allocate<SymbolList>();
		(yyval.symbollist_t)->push_back((yyvsp[(1) - (1)].symbol_t));
	;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 278 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.symbollist_t) = (yyvsp[(1) - (3)].symbollist_t);
		(yyval.symbollist_t)->push_back((yyvsp[(3) - (3)].symbol_t));
	;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 285 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.symbol_t) = interpreter->Allocate<symbol_desc>();

		(yyval.symbol_t)->name = *(yyvsp[(2) - (2)].text_t);
		(yyval.symbol_t)->type = (yyvsp[(1) - (2)].type_t);
		(yyval.symbol_t)->address = UNKNOWN_ADDR;

		interpreter->Deallocate((yyvsp[(2) - (2)].text_t));
	;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 297 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("statement_block -> epsilon");
		(yyval.statlist_t) = interpreter->Allocate<StatList>();
	;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 302 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("statement_block -> statement_block statement SEMICOLON");

		(yyval.statlist_t) = (yyvsp[(1) - (3)].statlist_t);
		(yyval.statlist_t)->push_back((yyvsp[(2) - (3)].stat_t));
	;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 309 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("statement_block -> statement_block control_block");

		(yyval.statlist_t) = (yyvsp[(1) - (2)].statlist_t);
		(yyval.statlist_t)->push_back((yyvsp[(2) - (2)].stat_t));
	;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 318 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("statement -> print");
		(yyval.stat_t) = (yyvsp[(1) - (1)].stat_t);
	;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 323 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("statement -> declaration");
		(yyval.stat_t) = (yyvsp[(1) - (1)].stat_t);
	;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 328 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		// even if it has no sense, like (5 + 3);
		PARSER_OUT("statement -> expr");

		(yyval.stat_t) = interpreter->Allocate<statement_desc>();
		(yyval.stat_t)->bytecode << (yyvsp[(1) - (1)].expr_t)->bytecode;

		interpreter->Deallocate((yyvsp[(1) - (1)].expr_t));
	;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 338 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("statement -> RETURN");
		symbol_desc* func = interpreter->current_func;

		INTERP_NERROR(0, "In function '" << func->name << "': Invalid return type", func->type != Type_Unknown);

		(yyval.stat_t) = interpreter->Allocate<statement_desc>();

		(yyval.stat_t)->type = Type_Return;
		(yyval.stat_t)->scope = interpreter->current_scope;

		(yyval.stat_t)->bytecode << OP(OP_MOV_RS) << REG(EAX) << (int64_t)0;
		(yyval.stat_t)->bytecode << OP(OP_MOV_RR) << REG(ESP) << REG(EBP);
		(yyval.stat_t)->bytecode << OP(OP_POP) << REG(EBP) << NIL;
		(yyval.stat_t)->bytecode << OP(OP_POP) << REG(EIP) << NIL;
	;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 355 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("statement -> RETURN expr");
		symbol_desc* func = interpreter->current_func;

		INTERP_NERROR(0, "In function '" << func->name << "': Invalid return type", func->type != (yyvsp[(2) - (2)].expr_t)->type);

		// mov the result into EAX and return
		(yyval.stat_t) = interpreter->Allocate<statement_desc>();

		(yyval.stat_t)->type = Type_Return;
		(yyval.stat_t)->scope = interpreter->current_scope;

		if ((yyvsp[(2) - (2)].expr_t)->isconst) {
			int64_t val = atoi((yyvsp[(2) - (2)].expr_t)->value.c_str());
			(yyval.stat_t)->bytecode << OP(OP_MOV_RS) << REG(EAX) << val;
		} else if ((yyvsp[(2) - (2)].expr_t)->address == UNKNOWN_ADDR) {
			(yyval.stat_t)->bytecode << (yyvsp[(2) - (2)].expr_t)->bytecode;
		} else {
			(yyval.stat_t)->bytecode << (yyvsp[(2) - (2)].expr_t)->bytecode;
			(yyval.stat_t)->bytecode << OP(OP_MOV_RM) << REG(EAX) << (yyvsp[(2) - (2)].expr_t)->address;
		}

		(yyval.stat_t)->bytecode << OP(OP_MOV_RR) << REG(ESP) << REG(EBP);
		(yyval.stat_t)->bytecode << OP(OP_POP) << REG(EBP) << NIL;
		(yyval.stat_t)->bytecode << OP(OP_POP) << REG(EIP) << NIL;

		interpreter->Deallocate((yyvsp[(2) - (2)].expr_t));
	;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 386 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.stat_t) = (yyvsp[(1) - (1)].stat_t);
	;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 390 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.stat_t) = (yyvsp[(1) - (1)].stat_t);
	;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 396 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.stat_t) = interpreter->Allocate<statement_desc>();

		if ((yyvsp[(3) - (5)].expr_t)->isconst) {
			int val = atoi((yyvsp[(3) - (5)].expr_t)->value.c_str());

			if (val != 0) {
				for (auto it = (yyvsp[(5) - (5)].statlist_t)->begin(); it != (yyvsp[(5) - (5)].statlist_t)->end(); ++it) {
					if (*it) {
						(yyval.stat_t)->bytecode << (*it)->bytecode;
						interpreter->Deallocate(*it);
					}
				}
			}
		} else {
			(yyval.stat_t)->bytecode << (yyvsp[(3) - (5)].expr_t)->bytecode;

			if ((yyvsp[(3) - (5)].expr_t)->address != UNKNOWN_ADDR)
				(yyval.stat_t)->bytecode << OP(OP_MOV_RM) << OP(EAX) << (yyvsp[(3) - (5)].expr_t)->address;
	
			int64_t off = 0;
	
			// calculate offset
			for (auto it = (yyvsp[(5) - (5)].statlist_t)->begin(); it != (yyvsp[(5) - (5)].statlist_t)->end(); ++it) {
				if (*it)
					off += (int64_t)(*it)->bytecode.Size();
			}
	
			(yyval.stat_t)->bytecode << OP(OP_JZ) << REG(EAX) << off;
	
			// apply true branch
			for (auto it = (yyvsp[(5) - (5)].statlist_t)->begin(); it != (yyvsp[(5) - (5)].statlist_t)->end(); ++it) {
				if (*it) {
					(yyval.stat_t)->bytecode << (*it)->bytecode;
					interpreter->Deallocate(*it);
				}
			}
		}

		interpreter->Deallocate((yyvsp[(3) - (5)].expr_t));
		interpreter->Deallocate((yyvsp[(5) - (5)].statlist_t));
	;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 439 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.stat_t) = interpreter->Allocate<statement_desc>();

		if ((yyvsp[(3) - (7)].expr_t)->isconst) {
			int val = atoi((yyvsp[(3) - (7)].expr_t)->value.c_str());

			if (val != 0) {
				// apply true branch
				for (auto it = (yyvsp[(5) - (7)].statlist_t)->begin(); it != (yyvsp[(5) - (7)].statlist_t)->end(); ++it) {
					if (*it) {
						(yyval.stat_t)->bytecode << (*it)->bytecode;
						interpreter->Deallocate(*it);
					}
				}
			} else {
				// apply false branch
				for (auto it = (yyvsp[(7) - (7)].statlist_t)->begin(); it != (yyvsp[(7) - (7)].statlist_t)->end(); ++it) {
					if (*it) {
						(yyval.stat_t)->bytecode << (*it)->bytecode;
						interpreter->Deallocate(*it);
					}
				}
			}
		} else {
			(yyval.stat_t)->bytecode << (yyvsp[(3) - (7)].expr_t)->bytecode;
	
			if ((yyvsp[(3) - (7)].expr_t)->address != UNKNOWN_ADDR)
				(yyval.stat_t)->bytecode << OP(OP_MOV_RM) << OP(EAX) << (yyvsp[(3) - (7)].expr_t)->address;
	
			int64_t off1 = 0;
			int64_t off2 = 0;
	
			// calculate offset
			for (auto it = (yyvsp[(5) - (7)].statlist_t)->begin(); it != (yyvsp[(5) - (7)].statlist_t)->end(); ++it) {
				if (*it)
					off1 += (int64_t)(*it)->bytecode.Size();
			}
	
			for (auto it = (yyvsp[(7) - (7)].statlist_t)->begin(); it != (yyvsp[(7) - (7)].statlist_t)->end(); ++it) {
				if (*it)
					off2 += (int64_t)(*it)->bytecode.Size();
			}
	
			// because the jump below adds some bytes
			(yyval.stat_t)->bytecode << OP(OP_JZ) << REG(EAX) << (int64_t)(off1 + ENTRY_SIZE);
	
			// apply true branch
			for (auto it = (yyvsp[(5) - (7)].statlist_t)->begin(); it != (yyvsp[(5) - (7)].statlist_t)->end(); ++it) {
				if (*it) {
					(yyval.stat_t)->bytecode << (*it)->bytecode;
					interpreter->Deallocate(*it);
				}
			}
	
			(yyval.stat_t)->bytecode << OP(OP_JMP) << off2 << NIL;
	
			// apply false branch
			for (auto it = (yyvsp[(7) - (7)].statlist_t)->begin(); it != (yyvsp[(7) - (7)].statlist_t)->end(); ++it) {
				if (*it) {
					(yyval.stat_t)->bytecode << (*it)->bytecode;
					interpreter->Deallocate(*it);
				}
			}
		}

		interpreter->Deallocate((yyvsp[(3) - (7)].expr_t));
		interpreter->Deallocate((yyvsp[(5) - (7)].statlist_t));
		interpreter->Deallocate((yyvsp[(7) - (7)].statlist_t));
	;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 511 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.stat_t) = interpreter->Allocate<statement_desc>();
		(yyval.stat_t)->bytecode << (yyvsp[(3) - (5)].expr_t)->bytecode;

		INTERP_NERROR(0, "Break statement not yet supported", (yyvsp[(3) - (5)].expr_t)->isconst);

		int64_t off = 0;
		int64_t loop = (yyvsp[(3) - (5)].expr_t)->bytecode.Size();

		if ((yyvsp[(3) - (5)].expr_t)->address != UNKNOWN_ADDR) {
			(yyval.stat_t)->bytecode << OP(OP_MOV_RM) << OP(EAX) << (yyvsp[(3) - (5)].expr_t)->address;
			loop += ENTRY_SIZE;
		}

		// calculate offset
		for (auto it = (yyvsp[(5) - (5)].statlist_t)->begin(); it != (yyvsp[(5) - (5)].statlist_t)->end(); ++it) {
			if (*it)
				off += (int64_t)(*it)->bytecode.Size();
		}

		(yyval.stat_t)->bytecode << OP(OP_JZ) << REG(EAX) << (int64_t)(off + ENTRY_SIZE);

		// jz and jmp
		loop += (off + 2 * ENTRY_SIZE);

		for (auto it = (yyvsp[(5) - (5)].statlist_t)->begin(); it != (yyvsp[(5) - (5)].statlist_t)->end(); ++it) {
			if (*it) {
				(yyval.stat_t)->bytecode << (*it)->bytecode;
				interpreter->Deallocate(*it);
			}
		}

		(yyval.stat_t)->bytecode << OP(OP_JMP) << -loop << NIL;

		interpreter->Deallocate((yyvsp[(3) - (5)].expr_t));
		interpreter->Deallocate((yyvsp[(5) - (5)].statlist_t));
	;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 551 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.statlist_t) = (yyvsp[(2) - (3)].statlist_t);

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
			(yyval.statlist_t)->push_back(dealloc);

			dealloc->bytecode << OP(OP_ADD_RS) << REG(ESP) << size;
		}

		--interpreter->current_scope;
	;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 580 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		++interpreter->current_scope;
	;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 586 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("print -> PRINT string");

		(yyval.stat_t) = interpreter->Allocate<statement_desc>();
		(yyval.stat_t)->bytecode << OP(OP_PRINT_M) << ADDR((yyvsp[(2) - (2)].text_t)) << REG(0);
	;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 593 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("print -> PRINT expr");
		(yyval.stat_t) = interpreter->Allocate<statement_desc>();

		if ((yyvsp[(2) - (2)].expr_t)->address == UNKNOWN_ADDR) {
			// result of an expression in EAX
			(yyval.stat_t)->bytecode << (yyvsp[(2) - (2)].expr_t)->bytecode << OP(OP_PRINT_R) << REG(EAX) << REG(0);
		} else {
			// variable on the stack
			(yyval.stat_t)->bytecode << (yyvsp[(2) - (2)].expr_t)->bytecode;
			(yyval.stat_t)->bytecode << OP(OP_MOV_RM) << REG(EAX) << (yyvsp[(2) - (2)].expr_t)->address;
			(yyval.stat_t)->bytecode << OP(OP_PRINT_R) << REG(EAX) << REG(0);
		}

		interpreter->Deallocate((yyvsp[(2) - (2)].expr_t));
	;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 612 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("declaration -> typename init_declarator_list");

		int scope = interpreter->current_scope;
		int64_t size = 0;

		SymbolTable& table = interpreter->scopes[scope];
		SymbolTable::iterator sym;
		symbol_desc* var;
		expression_desc* expr;

		(yyval.stat_t) = interpreter->Allocate<statement_desc>();

		// look for entries in current scope
		for (auto it = (yyvsp[(2) - (2)].decllist_t)->begin(); it != (yyvsp[(2) - (2)].decllist_t)->end(); ++it) {
			sym = table.find((*it)->name);
			INTERP_NERROR(0, "Conflicting declaration '" << (*it)->name << "'", sym != table.end());

			// save this variable
			var = interpreter->Allocate<symbol_desc>();
			var->type = (yyvsp[(1) - (2)].type_t);

			table[(*it)->name] = var;

			// update address
			interpreter->alloc_addr += interpreter->Sizeof((yyvsp[(1) - (2)].type_t));
			var->address = -interpreter->alloc_addr;

			// extend size
			size += interpreter->Sizeof((yyvsp[(1) - (2)].type_t));
		}

		(yyval.stat_t)->bytecode << OP(OP_SUB_RS) << REG(ESP) << size;

		for (auto it = (yyvsp[(2) - (2)].decllist_t)->begin(); it != (yyvsp[(2) - (2)].decllist_t)->end(); ++it) {
			sym = table.find((*it)->name);
			var = sym->second;
			expr = (*it)->expr;

			if (expr) {
				if (expr->isconst) {
					int64_t a = atoi(expr->value.c_str());

					(yyval.stat_t)->bytecode << OP(OP_MOV_RS) << REG(EAX) << a;
					(yyval.stat_t)->bytecode << expr->bytecode << OP(OP_MOV_MR) << var->address << REG(EAX);
				} else if (expr->address == UNKNOWN_ADDR) {
					// result of an expression in EAX
					(yyval.stat_t)->bytecode << expr->bytecode << OP(OP_MOV_MR) << var->address << REG(EAX);
				} else {
					// variable on the stack
					(yyval.stat_t)->bytecode << expr->bytecode;
					(yyval.stat_t)->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr->address;
					(yyval.stat_t)->bytecode << OP(OP_MOV_MR) << var->address << REG(EAX);
				}

				interpreter->Deallocate(expr);
			}

			interpreter->Deallocate(*it);
		}

		interpreter->Deallocate((yyvsp[(2) - (2)].decllist_t));
	;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 678 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("init_declarator_list -> init_declarator");

		(yyval.decllist_t) = interpreter->Allocate<DeclList>();
		(yyval.decllist_t)->push_back((yyvsp[(1) - (1)].decl_t));
	;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 685 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("init_declarator_list -> init_declarator_list COMMA init_declarator");

		(yyval.decllist_t) = (yyvsp[(1) - (3)].decllist_t);
		(yyval.decllist_t)->push_back((yyvsp[(3) - (3)].decl_t));
	;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 694 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("init_declarator -> IDENTIFIER");

		(yyval.decl_t) = interpreter->Allocate<declaration_desc>();
		(yyval.decl_t)->name = *(yyvsp[(1) - (1)].text_t);

		interpreter->Deallocate((yyvsp[(1) - (1)].text_t));
	;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 703 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("init_declarator -> IDENTIFIER EQ expr");

		(yyval.decl_t) = interpreter->Allocate<declaration_desc>();
		(yyval.decl_t)->name = *(yyvsp[(1) - (3)].text_t);
		(yyval.decl_t)->expr = (yyvsp[(3) - (3)].expr_t);

		interpreter->Deallocate((yyvsp[(1) - (3)].text_t));
	;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 715 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("expr -> assignment");
		(yyval.expr_t) = (yyvsp[(1) - (1)].expr_t);
	;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 722 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = (yyvsp[(1) - (1)].expr_t);
	;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 726 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("assignment -> lvalue = assignment");

		INTERP_ERROR(0, "assignment -> lvalue = assignment: NULL == $1", (yyvsp[(1) - (3)].expr_t));
		INTERP_ERROR(0, "assignment -> lvalue = assignment: NULL == $3", (yyvsp[(3) - (3)].expr_t));

		(yyval.expr_t) = (yyvsp[(1) - (3)].expr_t);

		if ((yyvsp[(3) - (3)].expr_t)->isconst) {
			int64_t a = atoi((yyvsp[(3) - (3)].expr_t)->value.c_str());

			(yyval.expr_t)->bytecode << OP(OP_MOV_RS) << REG(EAX) << a;
			(yyval.expr_t)->bytecode << (yyvsp[(3) - (3)].expr_t)->bytecode << OP(OP_MOV_MR) << (yyvsp[(1) - (3)].expr_t)->address << REG(EAX);
		} else if ((yyvsp[(3) - (3)].expr_t)->address == UNKNOWN_ADDR) {
			// result of an expression in EAX
			(yyval.expr_t)->bytecode << (yyvsp[(3) - (3)].expr_t)->bytecode << OP(OP_MOV_MR) << (yyvsp[(1) - (3)].expr_t)->address << REG(EAX);
		} else {
			// variable on the stack
			(yyval.expr_t)->bytecode << (yyvsp[(3) - (3)].expr_t)->bytecode << OP(OP_MOV_MM) << (yyvsp[(1) - (3)].expr_t)->address << (yyvsp[(3) - (3)].expr_t)->address;
		}

		interpreter->Deallocate((yyvsp[(3) - (3)].expr_t));
	;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 752 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = (yyvsp[(1) - (1)].expr_t);
	;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 756 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_OR_RR);
	;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 762 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = (yyvsp[(1) - (1)].expr_t);
	;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 766 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_AND_RR);
	;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 772 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = (yyvsp[(1) - (1)].expr_t);
	;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 776 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_SETE_RR);
	;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 780 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_SETNE_RR);
	;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 786 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = (yyvsp[(1) - (1)].expr_t);
	;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 790 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_SETL_RR);
	;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 794 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_SETLE_RR);
	;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 798 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_SETG_RR);
	;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 802 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_SETGE_RR);
	;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 808 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = (yyvsp[(1) - (1)].expr_t);
	;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 812 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_ADD_RR);
	;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 816 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_SUB_RR);
	;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 822 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = (yyvsp[(1) - (1)].expr_t);
	;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 826 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_MUL_RR);
	;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 830 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_DIV_RR);
	;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 834 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Arithmetic_Expr((yyvsp[(1) - (3)].expr_t), (yyvsp[(3) - (3)].expr_t), OP_MOD_RR);
	;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 840 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = (yyvsp[(1) - (1)].expr_t);
	;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 844 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Unary_Expr((yyvsp[(2) - (2)].expr_t), Expr_Inc);

		if (!(yyval.expr_t))
			return 0;
	;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 851 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Unary_Expr((yyvsp[(2) - (2)].expr_t), Expr_Dec);

		if (!(yyval.expr_t))
			return 0;
	;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 858 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = (yyvsp[(2) - (2)].expr_t);
	;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 862 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Unary_Expr((yyvsp[(2) - (2)].expr_t), Expr_Neg);

		if (!(yyval.expr_t))
			return 0;
	;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 869 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Unary_Expr((yyvsp[(2) - (2)].expr_t), Expr_Not);

		if (!(yyval.expr_t))
			return 0;
	;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 878 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.expr_t) = interpreter->Allocate<expression_desc>();

		(yyval.expr_t)->address = (yyvsp[(1) - (1)].symbol_t)->address;
		(yyval.expr_t)->isconst = false;
	;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 887 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("term -> (expr)");
		(yyval.expr_t) = (yyvsp[(2) - (3)].expr_t);
	;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 892 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("term -> literal");
		(yyval.expr_t) = (yyvsp[(1) - (1)].expr_t);
	;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 897 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("term -> variable");
		(yyval.expr_t) = interpreter->Allocate<expression_desc>();

		(yyval.expr_t)->address = (yyvsp[(1) - (1)].symbol_t)->address;
		(yyval.expr_t)->isconst = false;
		(yyval.expr_t)->type = (yyvsp[(1) - (1)].symbol_t)->type;
	;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 906 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("term -> func_call");
		(yyval.expr_t) = interpreter->Allocate<expression_desc>();

		(yyval.expr_t)->address = UNKNOWN_ADDR;
		(yyval.expr_t)->isconst = false;
		(yyval.expr_t)->type = (yyvsp[(1) - (1)].symbol_t)->type;

		expression_desc* expr;
		int64_t val;
		int count = 0;

		if ((yyvsp[(1) - (1)].symbol_t)->args) {
			for (auto it = (yyvsp[(1) - (1)].symbol_t)->args->rbegin(); it != (yyvsp[(1) - (1)].symbol_t)->args->rend(); ++it) {
				expr = (*it);

				if (expr->isconst) {
					val = atoi(expr->value.c_str());
					(yyval.expr_t)->bytecode << OP(OP_MOV_RS) << REG(EAX) << val;
				} else if (expr->address == UNKNOWN_ADDR) {
					(yyval.expr_t)->bytecode << expr->bytecode;
				} else {
					(yyval.expr_t)->bytecode << expr->bytecode;
					(yyval.expr_t)->bytecode << OP(OP_MOV_RM) << REG(EAX) << expr->address;
				}

				(yyval.expr_t)->bytecode << OP(OP_PUSH) << REG(EAX) << NIL;

				interpreter->Deallocate(expr);
				++count;
			}

			interpreter->Deallocate((yyvsp[(1) - (1)].symbol_t)->args);
		}

		unresolved_reference* ref = interpreter->Allocate<unresolved_reference>();
		ref->func = (yyvsp[(1) - (1)].symbol_t);

		(yyval.expr_t)->bytecode << OP(OP_PUSHADD) << REG(EIP) << (int64_t)(ENTRY_SIZE);
		(yyval.expr_t)->bytecode << OP(OP_JMP) << UNKNOWN_ADDR << ADDR(ref);

		// clear the stack
		for (int i = 0; i < count; ++i)
			(yyval.expr_t)->bytecode << OP(OP_POP) << REG(EDX) << NIL;
	;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 954 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		// look for this function in the global scope
		SymbolTable::iterator sym;

		SymbolTable& table = interpreter->scopes[0];
		sym = table.find(*(yyvsp[(1) - (4)].text_t));

		INTERP_NERROR(0, "Undeclared function '" << *(yyvsp[(1) - (4)].text_t) << "'", sym == table.end());
		INTERP_ERROR(0, "Referenced symbol '" << *(yyvsp[(1) - (4)].text_t) << "' is not a function", sym->second->isfunc);

		(yyval.symbol_t) = sym->second;
		(yyval.symbol_t)->args = (yyvsp[(3) - (4)].exprlist_t);

		interpreter->Deallocate((yyvsp[(1) - (4)].text_t));
	;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 970 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		// look for this function in the global scope
		SymbolTable::iterator sym;

		SymbolTable& table = interpreter->scopes[0];
		sym = table.find(*(yyvsp[(1) - (3)].text_t));

		INTERP_NERROR(0, "Undeclared function '" << *(yyvsp[(1) - (3)].text_t) << "'", sym == table.end());
		INTERP_ERROR(0, "Referenced symbol '" << *(yyvsp[(1) - (3)].text_t) << "' is not a function", sym->second->isfunc);

		(yyval.symbol_t) = sym->second;
		(yyval.symbol_t)->args = 0;

		interpreter->Deallocate((yyvsp[(1) - (3)].text_t));
	;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 988 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.exprlist_t) = interpreter->Allocate<ExprList>();
		(yyval.exprlist_t)->push_back((yyvsp[(1) - (1)].expr_t));
	;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 993 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		(yyval.exprlist_t) = (yyvsp[(1) - (3)].exprlist_t);
		(yyval.exprlist_t)->push_back((yyvsp[(3) - (3)].expr_t));
	;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1000 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("literal -> NUMBER");
		(yyval.expr_t) = interpreter->Allocate<expression_desc>();

		(yyval.expr_t)->type = Type_Integer;
		(yyval.expr_t)->value = *(yyvsp[(1) - (1)].text_t);
		(yyval.expr_t)->address = UNKNOWN_ADDR;
		(yyval.expr_t)->isconst = true;

		interpreter->Deallocate((yyvsp[(1) - (1)].text_t));
	;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1014 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("variable -> IDENTIFIER");

		// look for this variable in the symbol table
		bool found = false;
		SymbolTable::iterator sym;

		for (int s = interpreter->current_scope; s >= 0; --s) {
			SymbolTable& table = interpreter->scopes[s];
			sym = table.find(*(yyvsp[(1) - (1)].text_t));

			if (sym != table.end()) {
				found = true;
				break;
			}
		}

		INTERP_ERROR(0, "Undeclared identifier '" << *(yyvsp[(1) - (1)].text_t) << "'", found);
		(yyval.symbol_t) = sym->second;

		interpreter->Deallocate((yyvsp[(1) - (1)].text_t));
	;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1039 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("typename -> INT");
		(yyval.type_t) = Type_Integer;
	;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1044 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("typename -> VOID");
		(yyval.type_t) = Type_Unknown;
	;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1051 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"
    {
		PARSER_OUT("string -> QUOTE STRING QUOTE");
		(yyval.text_t) = (yyvsp[(2) - (3)].text_t);
	;}
    break;



/* Line 1455 of yacc.c  */
#line 2809 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 1057 "D:\\private\\Asylum_Refactoring\\OtherTutors\\61_AdvancedInterpreter/parser.y"


#ifdef _MSC_VER
#	pragma warning(pop)
#endif

