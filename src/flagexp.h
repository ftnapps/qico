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




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



