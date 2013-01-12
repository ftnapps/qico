#ifndef BISON_FLAGEXP_H
# define BISON_FLAGEXP_H

# ifndef YYSTYPE
#  define YYSTYPE int
#  define YYSTYPE_IS_TRIVIAL 1
# endif
# define	DATESTR	257
# define	GAPSTR	258
# define	PHSTR	259
# define	TIMESTR	260
# define	ADDRSTR	261
# define	PATHSTR	262
# define	ANYSTR	263
# define	IDENT	264
# define	NUMBER	265
# define	AROP	266
# define	LOGOP	267
# define	EQ	268
# define	NE	269
# define	GT	270
# define	GE	271
# define	LT	272
# define	LE	273
# define	AND	274
# define	OR	275
# define	NOT	276
# define	XOR	277
# define	LB	278
# define	RB	279
# define	COMMA	280
# define	ADDRESS	281
# define	ITIME	282
# define	CONNSTR	283
# define	SPEED	284
# define	CONNECT	285
# define	PHONE	286
# define	MAILER	287
# define	CID	288
# define	FLTIME	289
# define	FLDATE	290
# define	EXEC	291
# define	FLLINE	292
# define	PORT	293
# define	FLFILE	294
# define	HOST	295
# define	SFREE	296


extern YYSTYPE yylval;

#endif /* not BISON_FLAGEXP_H */
