/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     DATE = 258,
     DATESTR = 259,
     GAPSTR = 260,
     ITIME = 261,
     NUMBER = 262,
     PHSTR = 263,
     TIMESTR = 264,
     ADDRSTR = 265,
     IDENT = 266,
     CONNSTR = 267,
     SPEED = 268,
     CONNECT = 269,
     PHONE = 270,
     MAILER = 271,
     TIME = 272,
     ADDRESS = 273,
     FLEXEC = 274,
     FLLINE = 275,
     DOW = 276,
     ANY = 277,
     WK = 278,
     WE = 279,
     SUN = 280,
     MON = 281,
     TUE = 282,
     WED = 283,
     THU = 284,
     FRI = 285,
     SAT = 286,
     EQ = 287,
     NE = 288,
     GT = 289,
     GE = 290,
     LT = 291,
     LE = 292,
     LB = 293,
     RB = 294,
     AND = 295,
     OR = 296,
     NOT = 297,
     XOR = 298,
     COMMA = 299,
     ASTERISK = 300,
     AROP = 301,
     LOGOP = 302,
     PORT = 303,
     CID = 304,
     FLFILE = 305,
     PATHSTR = 306,
     HOST = 307,
     SFREE = 308,
     ANYSTR = 309
   };
#endif
#define DATE 258
#define DATESTR 259
#define GAPSTR 260
#define ITIME 261
#define NUMBER 262
#define PHSTR 263
#define TIMESTR 264
#define ADDRSTR 265
#define IDENT 266
#define CONNSTR 267
#define SPEED 268
#define CONNECT 269
#define PHONE 270
#define MAILER 271
#define TIME 272
#define ADDRESS 273
#define FLEXEC 274
#define FLLINE 275
#define DOW 276
#define ANY 277
#define WK 278
#define WE 279
#define SUN 280
#define MON 281
#define TUE 282
#define WED 283
#define THU 284
#define FRI 285
#define SAT 286
#define EQ 287
#define NE 288
#define GT 289
#define GE 290
#define LT 291
#define LE 292
#define LB 293
#define RB 294
#define AND 295
#define OR 296
#define NOT 297
#define XOR 298
#define COMMA 299
#define ASTERISK 300
#define AROP 301
#define LOGOP 302
#define PORT 303
#define CID 304
#define FLFILE 305
#define PATHSTR 306
#define HOST 307
#define SFREE 308
#define ANYSTR 309




/* Copy the first part of user declarations.  */
#line 11 "flagexp.y"

#include "headers.h"
#include <fnmatch.h>
#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
#ifdef NEED_DEBUG
#define YYERROR_VERBOSE 1
#endif

int yylex();
int flxpres;

static int logic(int e1, int op,int e2);
static int checkconnstr(void);
static int checkspeed(int op, int speed, int real);
static int checksfree(int op,int sp);
static int checkmailer(void);
static int checkphone(void);
static int checkport(void);
static int checkcid(void);
static int checkhost(void);
static int checkfile(void);
static int checkexec(void);
static int checkline(int lnum);
static int yyerror(char *s);
extern char *yyPTR;


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

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 225 "flagexp.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  45
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   53

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  55
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  8
/* YYNRULES -- Number of rules. */
#define YYNRULES  30
/* YYNRULES -- Number of states. */
#define YYNSTATES  59

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   309

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    14,    18,    20,    24,
      28,    33,    36,    39,    42,    45,    48,    51,    54,    57,
      60,    63,    66,    69,    72,    74,    76,    80,    82,    86,
      88
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      56,     0,    -1,    57,    -1,    58,    -1,    42,    57,    -1,
      57,    47,    57,    -1,    38,    57,    39,    -1,    59,    -1,
      14,    46,     7,    -1,    13,    46,     7,    -1,    53,    46,
       7,    51,    -1,    12,    12,    -1,    15,     8,    -1,    16,
      11,    -1,    49,     8,    -1,    52,    11,    -1,    48,    11,
      -1,    19,    54,    -1,    50,    51,    -1,    20,     7,    -1,
       6,    61,    -1,    17,    62,    -1,     3,    60,    -1,    18,
      10,    -1,    11,    -1,     4,    -1,     4,    44,    60,    -1,
       9,    -1,     9,    44,    61,    -1,     5,    -1,     5,    44,
      62,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned char yyrline[] =
{
       0,    43,    43,    46,    48,    50,    52,    55,    56,    58,
      60,    62,    64,    66,    68,    70,    72,    74,    76,    78,
      80,    82,    84,    86,    89,    92,    94,    97,    99,   102,
     104
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "DATE", "DATESTR", "GAPSTR", "ITIME", 
  "NUMBER", "PHSTR", "TIMESTR", "ADDRSTR", "IDENT", "CONNSTR", "SPEED", 
  "CONNECT", "PHONE", "MAILER", "TIME", "ADDRESS", "FLEXEC", "FLLINE", 
  "DOW", "ANY", "WK", "WE", "SUN", "MON", "TUE", "WED", "THU", "FRI", 
  "SAT", "EQ", "NE", "GT", "GE", "LT", "LE", "LB", "RB", "AND", "OR", 
  "NOT", "XOR", "COMMA", "ASTERISK", "AROP", "LOGOP", "PORT", "CID", 
  "FLFILE", "PATHSTR", "HOST", "SFREE", "ANYSTR", "$accept", "fullline", 
  "expression", "elemexp", "flag", "datestring", "timestring", 
  "gapstring", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    55,    56,    57,    57,    57,    57,    58,    58,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      58,    58,    58,    58,    59,    60,    60,    61,    61,    62,
      62
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     2,     3,     3,     1,     3,     3,
       4,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     1,     1,     3,     1,     3,     1,
       3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,    24,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     2,     3,     7,    25,    22,    27,    20,    11,     0,
       0,    12,    13,    29,    21,    23,    17,    19,     0,     4,
      16,    14,    18,    15,     0,     1,     0,     0,     0,     9,
       8,     0,     6,     0,     5,    26,    28,    30,    10
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    20,    21,    22,    23,    25,    27,    34
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -40
static const yysigned_char yypact[] =
{
      -3,     0,    -4,   -40,    -6,   -39,   -27,    12,    10,    17,
      13,   -30,    18,    -3,    -3,    16,    20,   -22,    19,   -15,
      32,   -14,   -40,   -40,    -8,   -40,    -7,   -40,   -40,    31,
      33,   -40,   -40,    -2,   -40,   -40,   -40,   -40,   -21,   -14,
     -40,   -40,   -40,   -40,    34,   -40,    -3,     0,    -4,   -40,
     -40,    17,   -40,     1,   -14,   -40,   -40,   -40,   -40
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -40,   -40,   -12,   -40,   -40,     4,    -5,     2
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
       1,    38,    39,     2,    24,    26,    28,    29,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    52,    30,
      31,    32,    33,    35,    36,    37,    46,    40,    41,    42,
      43,    44,    45,    46,    54,    13,    47,    48,    49,    14,
      50,    53,    51,    56,     0,    15,    16,    17,     0,    18,
      19,    55,    58,    57
};

static const yysigned_char yycheck[] =
{
       3,    13,    14,     6,     4,     9,    12,    46,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    39,    46,
       8,    11,     5,    10,    54,     7,    47,    11,     8,    51,
      11,    46,     0,    47,    46,    38,    44,    44,     7,    42,
       7,     7,    44,    48,    -1,    48,    49,    50,    -1,    52,
      53,    47,    51,    51
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     6,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    38,    42,    48,    49,    50,    52,    53,
      56,    57,    58,    59,     4,    60,     9,    61,    12,    46,
      46,     8,    11,     5,    62,    10,    54,     7,    57,    57,
      11,     8,    51,    11,    46,     0,    47,    44,    44,     7,
       7,    44,    39,     7,    57,    60,    61,    62,    51
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

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
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
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
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
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
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

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
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
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

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 44 "flagexp.y"
    {flxpres=yyvsp[0];}
    break;

  case 3:
#line 47 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 4:
#line 49 "flagexp.y"
    {yyval = !(yyvsp[0]);}
    break;

  case 5:
#line 51 "flagexp.y"
    {yyval = logic(yyvsp[-2],yyvsp[-1],yyvsp[0]);}
    break;

  case 6:
#line 53 "flagexp.y"
    {yyval = yyvsp[-1];}
    break;

  case 8:
#line 57 "flagexp.y"
    {yyval = checkspeed(yyvsp[-1],yyvsp[0],1);}
    break;

  case 9:
#line 59 "flagexp.y"
    {yyval = checkspeed(yyvsp[-1],yyvsp[0],0);}
    break;

  case 10:
#line 61 "flagexp.y"
    {yyval = checksfree(yyvsp[-2],yyvsp[-1]);}
    break;

  case 11:
#line 63 "flagexp.y"
    {yyval = checkconnstr();}
    break;

  case 12:
#line 65 "flagexp.y"
    {yyval = checkphone();}
    break;

  case 13:
#line 67 "flagexp.y"
    {yyval = checkmailer();}
    break;

  case 14:
#line 69 "flagexp.y"
    {yyval = checkcid();}
    break;

  case 15:
#line 71 "flagexp.y"
    {yyval = checkhost();}
    break;

  case 16:
#line 73 "flagexp.y"
    {yyval = checkport();}
    break;

  case 17:
#line 75 "flagexp.y"
    {yyval = checkexec();}
    break;

  case 18:
#line 77 "flagexp.y"
    {yyval = checkfile();}
    break;

  case 19:
#line 79 "flagexp.y"
    {yyval = checkline(yyvsp[0]);}
    break;

  case 20:
#line 81 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 21:
#line 83 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 22:
#line 85 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 23:
#line 87 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 24:
#line 90 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 25:
#line 93 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 26:
#line 95 "flagexp.y"
    {yyval = logic(yyvsp[-2],OR,yyvsp[0]);}
    break;

  case 27:
#line 98 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 28:
#line 100 "flagexp.y"
    {yyval = logic(yyvsp[-2],OR,yyvsp[0]);}
    break;

  case 29:
#line 103 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 30:
#line 105 "flagexp.y"
    {yyval = logic(yyvsp[-2],OR,yyvsp[0]);}
    break;


    }

/* Line 991 of yacc.c.  */
#line 1288 "flagexp.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


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
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR. */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__) && !defined(__c_plusplus)
  __attribute__ ((__unused__))
#endif 


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
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

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 107 "flagexp.y"


static int logic(int e1, int op,int e2)
{
	DEBUG(('Y',2,"Logic: %d (%d,%s) %d",e1,op,
		(AND==op?"AND":
		(OR==op?"OR":
		(XOR==op?"XOR":"???"
		))),e2));
	switch (op)
	{
	case AND:	return(e1 && e2);
	case OR:	return(e1 || e2);
	case XOR:	return(e1 ^ e2)?1:0;
	default:
		DEBUG(('Y',1,"Logic: invalid logical operator %d",op));
		return 0;
	}
}

static int checkconnstr(void)
{
	DEBUG(('Y',2,"checkconnstr: \"%s\"",yytext));
	if(!connstr||is_ip) return 0;
	DEBUG(('Y',3,"checkconnstr: \"%s\" <-> \"%s\"",yytext,connstr));
	if(!strstr(connstr,yytext)) return 1;
	return 0;
}

static int checkspeed(int op, int speed, int real)
{
	DEBUG(('Y',2,"checkspeed: \"%s\"",yytext));
	if(!rnode) return 0;
	DEBUG(('Y',3,"check%sspeed: %d (%d,%s) %d",real?"real":"",real?rnode->realspeed:rnode->speed,op,
		(EQ==op?"==":
		(NE==op?"!=":
	        (GT==op?">":
	        (GE==op?">=":
	        (LT==op?"<":
	        (LE==op?"<=":"???"
		)))))),speed));
	switch (op)
	{
	case EQ:	return(real?rnode->realspeed:rnode->speed == speed);
	case NE:	return(real?rnode->realspeed:rnode->speed != speed);
	case GT:	return(real?rnode->realspeed:rnode->speed >  speed);
	case GE:	return(real?rnode->realspeed:rnode->speed >= speed);
	case LT:	return(real?rnode->realspeed:rnode->speed <  speed);
	case LE:	return(real?rnode->realspeed:rnode->speed <= speed);
	default:
		DEBUG(('Y',1,"Logic: invalid comparsion operator %d",op));
		return 0;
	}
}

static int checksfree(int op,int sf)
{
	int fs=getfreespace((const char*)yytext);
	DEBUG(('Y',2,"checksfree: '%s' %d (%d,%s) %d",yytext,fs,op,
	        (GT==op?">":(GE==op?">=":
	        (LT==op?"<":(LE==op?"<=":"???")))),sf));
	switch (op) {
	case GT:	return(fs >  sf);
	case GE:	return(fs >= sf);
	case LT:	return(fs <  sf);
	case LE:	return(fs <= sf);
	default:
		DEBUG(('Y',1,"Logic: invalid comparsion operator %d",op));
		return 0;
	}
}

static int checkphone(void)
{
	DEBUG(('Y',2,"checkphone: \"%s\"",yytext));
	if(!rnode||!rnode->phone) return 0;
	DEBUG(('Y',3,"checkphone: \"%s\" <-> \"%s\"",yytext,rnode->phone));
	if(!strncasecmp(yytext,rnode->phone,strlen(yytext))) return 1;
	return 0;
}

static int checkmailer(void)
{
	DEBUG(('Y',2,"checkmailer: \"%s\"",yytext));
	if(!rnode||!rnode->mailer) return 0;
	DEBUG(('Y',3,"checkmailer: \"%s\" <-> \"%s\"",yytext,rnode->mailer));
	if(!strstr(rnode->mailer,yytext)) return 1;
	return 0;
}

static int checkcid(void)
{
	char *cid = getenv("CALLER_ID");
	if(!cid||strlen(cid)<4) cid = "none";
	DEBUG(('Y',2,"checkcid: \"%s\" <-> \"%s\"",yytext,cid));
	if(!strncasecmp(yytext,cid,strlen(yytext))) return 1;
	return 0;
}

static int checkhost(void)
{
	DEBUG(('Y',2,"checkhost: \"%s\"",yytext));
	if(!rnode || !rnode->host) return 0;
	DEBUG(('Y',3,"checkhost: \"%s\" <-> \"%s\"",yytext,rnode->host));
	if(!strncasecmp(yytext,rnode->host,strlen(yytext)))return 1;
	return 0;
}

static int checkport(void)
{
	DEBUG(('Y',2,"checkport: \"%s\"",yytext));
	if(!rnode || !rnode->tty) return 0;
	DEBUG(('Y',3,"checkport: \"%s\" <-> \"%s\"",yytext,rnode->tty));
	if(!fnmatch(yytext,rnode->tty,FNM_NOESCAPE|FNM_PATHNAME)) return 1;
	return 0;
}

static int checkfile(void)
{
	struct stat sb;
	DEBUG(('Y',2,"checkfile: \"%s\" -> %d",yytext,!stat(yytext,&sb)));
	if(!stat(yytext,&sb)) return 1;
	return 0;
}

static int checkexec(void)
{
	int rc;
	char *cmd=xstrdup(yytext);
	DEBUG(('Y',2,"checkexec: \"%s\"",yytext));
	strtr(cmd,',',' ');
	rc=execsh(cmd);
	DEBUG(('Y',3,"checkexec: \"%s\" -> %d",cmd,!rc));
	xfree(cmd);
	return !rc;
}

static int checkline(int lnum)
{
	DEBUG(('Y',2,"checkline: \"%s\"",yytext));
	if(!rnode) return 0;
	DEBUG(('Y',3,"checkline: %d <-> %d",lnum,rnode->hidnum));
	if(rnode->hidnum==lnum)return 1;
	return 0;
}

int yyparse();

int flagexp(slist_t *expr,int strict)
{
	char *p;
	for(;expr;expr=expr->next) {
		DEBUG(('Y',1,"checkexpression: \"%s\"",expr->str));
		p=xstrdup(expr->str);
		yyPTR=p;
		flxpres=0;
		if(yyparse()) {
			DEBUG(('Y',1,"checkexpression: couldn't parse%s",strict?"":", assume 'false'",expr->str));
			xfree(p);
			return(strict?-1:0);
		}
#ifdef NEED_DEBUG
		if(!strict)DEBUG(('Y',1,"checkexpression: result is \"%s\"",flxpres?"true":"false"));
#endif
		xfree(p);
		if(!flxpres)return 0;
	}
	return 1;
}

static int yyerror(char *s)
{
	DEBUG(('Y',1,"yyerror: %s",s));
	return 0;
}

