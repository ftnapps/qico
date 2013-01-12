/* A Bison parser, made from flagexp.y, by GNU bison 1.75.  */

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
#define YYBISON	1

/* Pure parsers.  */
#define YYPURE	0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     DATESTR = 258,
     GAPSTR = 259,
     PHSTR = 260,
     TIMESTR = 261,
     ADDRSTR = 262,
     PATHSTR = 263,
     ANYSTR = 264,
     IDENT = 265,
     NUMBER = 266,
     AROP = 267,
     LOGOP = 268,
     EQ = 269,
     NE = 270,
     GT = 271,
     GE = 272,
     LT = 273,
     LE = 274,
     AND = 275,
     OR = 276,
     NOT = 277,
     XOR = 278,
     LB = 279,
     RB = 280,
     COMMA = 281,
     ADDRESS = 282,
     ITIME = 283,
     CONNSTR = 284,
     SPEED = 285,
     CONNECT = 286,
     PHONE = 287,
     MAILER = 288,
     CID = 289,
     FLTIME = 290,
     FLDATE = 291,
     EXEC = 292,
     FLLINE = 293,
     PORT = 294,
     FLFILE = 295,
     HOST = 296,
     SFREE = 297
   };
#endif
#define DATESTR 258
#define GAPSTR 259
#define PHSTR 260
#define TIMESTR 261
#define ADDRSTR 262
#define PATHSTR 263
#define ANYSTR 264
#define IDENT 265
#define NUMBER 266
#define AROP 267
#define LOGOP 268
#define EQ 269
#define NE 270
#define GT 271
#define GE 272
#define LT 273
#define LE 274
#define AND 275
#define OR 276
#define NOT 277
#define XOR 278
#define LB 279
#define RB 280
#define COMMA 281
#define ADDRESS 282
#define ITIME 283
#define CONNSTR 284
#define SPEED 285
#define CONNECT 286
#define PHONE 287
#define MAILER 288
#define CID 289
#define FLTIME 290
#define FLDATE 291
#define EXEC 292
#define FLLINE 293
#define PORT 294
#define FLFILE 295
#define HOST 296
#define SFREE 297




/* Copy the first part of user declarations.  */
#line 16 "flagexp.y"

#include "headers.h"
#include <fnmatch.h>

#ifdef NEED_DEBUG
#define YYERROR_VERBOSE 1
#endif
#define YYDEBUG 0

#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
extern char *yyPTR;
extern int yylex();
static int logic(int e1,int op,int e2);
static int checkflag(void);
static int checkconnstr(void);
static int checkspeed(int op, int speed, int real);
static int checksfree(int op, int sp);
static int checkmailer(void);
static int checkphone(void);
static int checkport(void);
static int checkcid(void);
static int checkhost(void);
static int checkfile(void);
static int checkexec(void);
static int checkline(int lnum);
static int yyerror(char *s);
static int flxpres;


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

#ifndef YYSTYPE
typedef int yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif

#ifndef YYLTYPE
typedef struct yyltype
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} yyltype;
# define YYLTYPE yyltype
# define YYLTYPE_IS_TRIVIAL 1
#endif

/* Copy the second part of user declarations.  */


/* Line 213 of /usr/local/share/bison/yacc.c.  */
#line 210 "flagexp.c"

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
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)

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
	    (To)[yyi] = (From)[yyi];	\
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
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
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
#define YYLAST   54

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  43
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  8
/* YYNRULES -- Number of rules. */
#define YYNRULES  30
/* YYNRULES -- Number of states. */
#define YYNSTATES  59

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   297

#define YYTRANSLATE(X) \
  ((unsigned)(X) <= YYMAXUTOK ? yytranslate[X] : YYUNDEFTOK)

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
      35,    36,    37,    38,    39,    40,    41,    42
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
      44,     0,    -1,    45,    -1,    46,    -1,    22,    45,    -1,
      45,    13,    45,    -1,    24,    45,    25,    -1,    47,    -1,
      31,    12,    11,    -1,    30,    12,    11,    -1,    42,    12,
      11,     8,    -1,    29,    29,    -1,    32,     5,    -1,    33,
      10,    -1,    34,     5,    -1,    41,    10,    -1,    39,    10,
      -1,    37,     9,    -1,    40,     8,    -1,    38,    11,    -1,
      28,    49,    -1,    35,    50,    -1,    36,    48,    -1,    27,
       7,    -1,    10,    -1,     3,    -1,     3,    26,    48,    -1,
       6,    -1,     6,    26,    49,    -1,     4,    -1,     4,    26,
      50,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned char yyrline[] =
{
       0,    56,    56,    59,    61,    63,    65,    68,    69,    71,
      73,    75,    77,    79,    81,    83,    85,    87,    89,    91,
      93,    95,    97,    99,   102,   105,   107,   110,   112,   115,
     117
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "DATESTR", "GAPSTR", "PHSTR", "TIMESTR", 
  "ADDRSTR", "PATHSTR", "ANYSTR", "IDENT", "NUMBER", "AROP", "LOGOP", 
  "EQ", "NE", "GT", "GE", "LT", "LE", "AND", "OR", "NOT", "XOR", "LB", 
  "RB", "COMMA", "ADDRESS", "ITIME", "CONNSTR", "SPEED", "CONNECT", 
  "PHONE", "MAILER", "CID", "FLTIME", "FLDATE", "EXEC", "FLLINE", "PORT", 
  "FLFILE", "HOST", "SFREE", "$accept", "fullline", "expression", 
  "elemexp", "flag", "datestring", "timestring", "gapstring", 0
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
     295,   296,   297
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    43,    44,    45,    45,    45,    45,    46,    46,    46,
      46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
      46,    46,    46,    46,    47,    48,    48,    49,    49,    50,
      50
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
       0,    24,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     2,     3,     7,     4,     0,    23,    27,    20,    11,
       0,     0,    12,    13,    14,    29,    21,    25,    22,    17,
      19,    16,    18,    15,     0,     1,     0,     6,     0,     9,
       8,     0,     0,     0,     5,    28,    30,    26,    10
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    20,    21,    22,    23,    38,    28,    36
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -24
static const yysigned_char yypact[] =
{
      -8,   -24,    -8,    -8,    -3,    -1,   -23,    -5,    -4,     4,
       0,     6,     8,    10,     9,    24,     7,    28,    27,    26,
      39,    29,   -24,   -24,    29,   -10,   -24,    14,   -24,   -24,
      30,    32,   -24,   -24,   -24,    19,   -24,    20,   -24,   -24,
     -24,   -24,   -24,   -24,    36,   -24,    -8,   -24,    -1,   -24,
     -24,     8,    10,    40,    29,   -24,   -24,   -24,   -24
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -24,   -24,    -2,   -24,   -24,     1,     2,     3
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, parse error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      24,    25,     1,    46,    26,    27,    29,    30,    31,    32,
      33,    34,    35,    37,     2,    47,     3,    41,    39,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    40,    42,    43,    44,    45,
      48,    49,    46,    50,    54,    51,    52,    53,    58,     0,
      55,     0,     0,    57,    56
};

static const yysigned_char yycheck[] =
{
       2,     3,    10,    13,     7,     6,    29,    12,    12,     5,
      10,     5,     4,     3,    22,    25,    24,    10,     9,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    11,     8,    10,    12,     0,
      26,    11,    13,    11,    46,    26,    26,    11,     8,    -1,
      48,    -1,    -1,    52,    51
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    10,    22,    24,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      44,    45,    46,    47,    45,    45,     7,     6,    49,    29,
      12,    12,     5,    10,     5,     4,    50,     3,    48,     9,
      11,    10,     8,    10,    12,     0,    13,    25,    26,    11,
      11,    26,    26,    11,    45,    49,    50,    48,     8
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
#define YYEMPTY		-2
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
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)           \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#define YYLEX	yylex ()

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
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
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
/*-----------------------------.
| Print this symbol on YYOUT.  |
`-----------------------------*/

static void
#if defined (__STDC__) || defined (__cplusplus)
yysymprint (FILE* yyout, int yytype, YYSTYPE yyvalue)
#else
yysymprint (yyout, yytype, yyvalue)
    FILE* yyout;
    int yytype;
    YYSTYPE yyvalue;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvalue;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyout, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyout, yytoknum[yytype], yyvalue);
# endif
    }
  else
    YYFPRINTF (yyout, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyout, ")");
}
#endif /* YYDEBUG. */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
#if defined (__STDC__) || defined (__cplusplus)
yydestruct (int yytype, YYSTYPE yyvalue)
#else
yydestruct (yytype, yyvalue)
    int yytype;
    YYSTYPE yyvalue;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvalue;

  switch (yytype)
    {
      default:
        break;
    }
}



/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of parse errors so far.  */
int yynerrs;


int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

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

  if (yyssp >= yyss + yystacksize - 1)
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
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
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

      if (yyssp >= yyss + yystacksize - 1)
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

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with.  */

  if (yychar <= 0)		/* This means end of input.  */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more.  */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

      /* We have to keep this `#if YYDEBUG', since we use variables
	 which are defined only if `YYDEBUG' is set.  */
      YYDPRINTF ((stderr, "Next token is "));
      YYDSYMPRINT ((stderr, yychar1, yylval));
      YYDPRINTF ((stderr, "\n"));
    }

  /* If the proper action on seeing token YYCHAR1 is to reduce or to
     detect an error, take that action.  */
  yyn += yychar1;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yychar1)
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
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

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



#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn - 1, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] >= 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif
  switch (yyn)
    {
        case 2:
#line 57 "flagexp.y"
    {flxpres=yyvsp[0];}
    break;

  case 3:
#line 60 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 4:
#line 62 "flagexp.y"
    {yyval = !(yyvsp[0]);}
    break;

  case 5:
#line 64 "flagexp.y"
    {yyval = logic(yyvsp[-2],yyvsp[-1],yyvsp[0]);}
    break;

  case 6:
#line 66 "flagexp.y"
    {yyval = yyvsp[-1];}
    break;

  case 8:
#line 70 "flagexp.y"
    {yyval = checkspeed(yyvsp[-1],yyvsp[0],1);}
    break;

  case 9:
#line 72 "flagexp.y"
    {yyval = checkspeed(yyvsp[-1],yyvsp[0],0);}
    break;

  case 10:
#line 74 "flagexp.y"
    {yyval = checksfree(yyvsp[-2],yyvsp[-1]);}
    break;

  case 11:
#line 76 "flagexp.y"
    {yyval = checkconnstr();}
    break;

  case 12:
#line 78 "flagexp.y"
    {yyval = checkphone();}
    break;

  case 13:
#line 80 "flagexp.y"
    {yyval = checkmailer();}
    break;

  case 14:
#line 82 "flagexp.y"
    {yyval = checkcid();}
    break;

  case 15:
#line 84 "flagexp.y"
    {yyval = checkhost();}
    break;

  case 16:
#line 86 "flagexp.y"
    {yyval = checkport();}
    break;

  case 17:
#line 88 "flagexp.y"
    {yyval = checkexec();}
    break;

  case 18:
#line 90 "flagexp.y"
    {yyval = checkfile();}
    break;

  case 19:
#line 92 "flagexp.y"
    {yyval = checkline(yyvsp[0]);}
    break;

  case 20:
#line 94 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 21:
#line 96 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 22:
#line 98 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 23:
#line 100 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 24:
#line 103 "flagexp.y"
    {yyval = checkflag();}
    break;

  case 25:
#line 106 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 26:
#line 108 "flagexp.y"
    {yyval = logic(yyvsp[-2],OR,yyvsp[0]);}
    break;

  case 27:
#line 111 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 28:
#line 113 "flagexp.y"
    {yyval = logic(yyvsp[-2],OR,yyvsp[0]);}
    break;

  case 29:
#line 116 "flagexp.y"
    {yyval = yyvsp[0];}
    break;

  case 30:
#line 118 "flagexp.y"
    {yyval = logic(yyvsp[-2],OR,yyvsp[0]);}
    break;


    }

/* Line 1016 of /usr/local/share/bison/yacc.c.  */
#line 1209 "flagexp.c"

  yyvsp -= yylen;
  yyssp -= yylen;


#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

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
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
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
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:
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
	  while (yyssp > yyss)
	    {
	      YYDPRINTF ((stderr, "Error: popping "));
	      YYDSYMPRINT ((stderr,
			    yystos[*yyssp],
			    *yyvsp));
	      YYDPRINTF ((stderr, "\n"));
	      yydestruct (yystos[*yyssp], *yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yydestruct (yychar1, yylval);
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

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

      YYDPRINTF ((stderr, "Error: popping "));
      YYDSYMPRINT ((stderr,
		    yystos[*yyssp], *yyvsp));
      YYDPRINTF ((stderr, "\n"));

      yydestruct (yystos[yystate], *yyvsp);
      yyvsp--;
      yystate = *--yyssp;


#if YYDEBUG
      if (yydebug)
	{
	  short *yyssp1 = yyss - 1;
	  YYFPRINTF (stderr, "Error: state stack now");
	  while (yyssp1 != yyssp)
	    YYFPRINTF (stderr, " %d", *++yyssp1);
	  YYFPRINTF (stderr, "\n");
	}
#endif
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


#line 120 "flagexp.y"


static int logic(int e1, int op, int e2)
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

static int checkflag(void)
{
	int fln;
	char *p, *q;
	DEBUG(('Y',2,"checkflag: \"%s\"",yytext));
#ifdef WITH_PERL
	if(!strncasecmp(yytext,"perl",4)) {
		if((fln=atoi(yytext+4))<0||fln>9) {
			write_log("error: invalid perl flag: %s",yytext);
			return 0;
		}
		DEBUG(('Y',3,"checkflag: perl%d: %d",fln,(perl_flg&(1<<fln))?1:0));
		return((perl_flg&(1<<fln))?1:0);
	}
#endif
	if(!rnode)return 0;
 	if(!strncasecmp(yytext,"list",4)) {
		DEBUG(('Y',3,"checkflag: listed: %d",rnode->options&O_LST));
		return rnode->options&O_LST;
	}
	if(!strncasecmp(yytext,"prot",4)) {
		DEBUG(('Y',3,"checkflag: protected: %d",rnode->options&O_PWD));
		return rnode->options&O_PWD;
	}
	if(!strncasecmp(yytext,"in",2)) {
		DEBUG(('Y',3,"checkflag: inbound: %d",rnode->options&O_INB));
		return rnode->options&O_INB;
	}
	if(!strncasecmp(yytext,"out",3)) {
		DEBUG(('Y',3,"checkflag: outbound: %d",!(rnode->options&O_INB)));
		return !(rnode->options&O_INB);
	}
	if(!strncasecmp(yytext,"tcp",3)) {
		DEBUG(('Y',3,"checkflag: tcp/ip: %d",rnode->options&O_TCP));
		return rnode->options&O_TCP;
	}
	if(!strncasecmp(yytext,"binkp",5)) {
		DEBUG(('Y',3,"checkflag: binkp: %d",bink));
		return bink;
	}
	if(!strncasecmp(yytext,"bad",3)) {
		DEBUG(('Y',3,"checkflag: bad password: %d",rnode->options&O_BAD));
		return rnode->options&O_BAD;
	}
	if(rnode->flags) {
		char *r;
		q = xstrdup(rnode->flags);
		r = q;;
		while(( p = strsep( &r, "," ))) {
			if(!strcasecmp(yytext,p)) {
				xfree(q);
				DEBUG(('Y',3,"checkflag: other: 1"));
				return 1;
			}
		}
		xfree(q);
	}
	DEBUG(('Y',3,"checkflag: other: 0"));
	return 0;
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

static int checksfree(int op, int sf)
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

int flagexp(slist_t *expr, int strict)
{
	char *p;
#if YYDEBUG==1
	yydebug=1;
#endif
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
		if(strict!=1)DEBUG(('Y',1,"checkexpression: result is \"%s\"",flxpres?"true":"false"));
#endif
		xfree(p);
		if(!flxpres)return 0;
	}
	return 1;
}

static int yyerror(char *s)
{
	DEBUG(('Y',1,"yyerror: %s at %s",s,(yytext&&*yytext)?yytext:"end of input"));
	return 0;
}

