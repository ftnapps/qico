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

#ifndef BISON_FLAGEXP_H
# define BISON_FLAGEXP_H

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




#ifndef YYSTYPE
typedef int yystype;
# define YYSTYPE yystype
#endif

extern YYSTYPE yylval;


#endif /* not BISON_FLAGEXP_H */

