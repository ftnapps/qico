/**********************************************************
 * File: flagexp.y
 * Created at Thu Jul 15 16:14:46 1999 by pk // aaz@ruxy.org.ru
 * Base version of this file was taken from Eugene Crosser's ifcico 
 * $Id: flagexp.y,v 1.1.1.1 2000/07/18 12:37:19 lev Exp $
 **********************************************************/
%token DATESTR GAPSTR ITIME NUMBER PHSTR TIMESTR ADDRSTR IDENT SPEED CONNECT PHONE TIME ADDRESS DOW ANY WK WE SUN MON TUE WED THU FRI SAT EQ NE GT GE LT LE LB RB AND OR NOT XOR COMMA ASTERISK AROP LOGOP PORT CID
%{
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fnmatch.h>
#include "ftn.h"
#include "mailer.h"	
#include "qcconst.h"
#include "globals.h"

#define YY_NO_UNPUT
#undef ECHO
	
int flxpres;
struct tm *now;
	

static int logic(int e1, int op,int e2);
static int checkspeed(int op, int speed, int real);
static int checkphone(void);
static int checkport(void);
static int checkcid(void);
static int yyerror(char *s);

%}


%%
fullline	: expression
                {flxpres=$1;}    
		;
expression	: elemexp
			{$$ = $1;}
		| NOT expression
		        {$$ = !($2);}
		| expression LOGOP expression
			{$$ = logic($1,$2,$3);}      
		| LB expression RB
			{$$ = $2;}
		;
elemexp		: flag
		| CONNECT AROP NUMBER
			{$$ = checkspeed($2,$3,1);}
		| SPEED AROP NUMBER
			{$$ = checkspeed($2,$3,0);}
		| PHONE PHSTR
			{$$ = checkphone();}
		| CID IDENT
			{$$ = checkcid();}
		| PORT IDENT
			{$$ = checkport();}
		| PHONE NUMBER
			{$$ = checkphone();}
		| ITIME timestring
			{$$ = $2;}
		| TIME gapstring
			{$$ = $2;}
		| ADDRESS ADDRSTR
			{$$ = $2;}
		;
flag		:IDENT
		{$$ = $1;}
		;
timestring	: TIMESTR
			{$$ = $1;}
		| TIMESTR COMMA timestring
			{$$ = logic($1,OR,$3);}
		;	
gapstring	: GAPSTR
			{$$ = $1;}
		| GAPSTR COMMA gapstring
			{$$ = logic($1,OR,$3);}
%%

#include "flaglex.c"
/* static int match(int fl) */
/* { */
/* 	int i; */

/* #ifdef Y_DEBUG	 */
/* 	log("match: %d",fl); */
/* #endif */
/* 	if (fl == -1) */
/* 	{ */
/* 		for (i=0;(i<MAXUFLAGS) && (nodebuf->uflags[i]);i++) */
/* 			if (strcasecmp(yytext,nodebuf->uflags[i]) == 0)  */
/* 				return 1; */
/* 		return 0; */
/* 	} */
/* 	else */
/* 	{ */
/* 		return ((nodebuf->flags & fl) != 0); */
/* 	} */
/* } */

static int logic(int e1, int op,int e2)
{
#ifdef Y_DEBUG
	log("logic: %d %d %d",e1,op,e2);
#endif
	switch (op)
	{
	case AND:	return(e1 && e2);
	case OR:	return(e1 || e2);
	case XOR:	return(e1 ^ e2);
	default:
#ifdef Y_DEBUG
		log("Parser: internal error: invalid logical operator");
#endif
		return 0;
	}
}

static int checkspeed(int op, int speed, int real)
{
#ifdef Y_DEBUG
	log("check%sspeed: %d %d",real?"real":"",op,speed);
#endif
	if(!rnode) return 0;
	switch (op)
	{
	case EQ:	return(real?rnode->realspeed:rnode->speed == speed);
	case NE:	return(real?rnode->realspeed:rnode->speed != speed);
	case GT:	return(real?rnode->realspeed:rnode->speed >  speed);
	case GE:	return(real?rnode->realspeed:rnode->speed >= speed);
	case LT:	return(real?rnode->realspeed:rnode->speed <  speed);
	case LE:	return(real?rnode->realspeed:rnode->speed <= speed);
	default:
#ifdef Y_DEBUG
		log("Parser: internal error: invalid arithmetic operator");
#endif
		return 0;
	}
}

static int checkphone(void)
{
#ifdef Y_DEBUG
	log("checkphone: \"%s\"",yytext);
#endif
	if(!rnode) return 0;
	if(rnode->phone == NULL) return 0;
	if(strncasecmp(yytext,rnode->phone,strlen(yytext)) == 0) return 1;
	else return 0;
}

static int checkcid(void)
{
#ifdef Y_DEBUG
	log("checkcid: \"%s\"",yytext);
#endif
	if(strncasecmp(yytext,getenv("CALLER_ID"),strlen(yytext)) == 0) return 1;
	else return 0;
}

static int checkport(void)
{
#ifdef Y_DEBUG
	log("checkport: \"%s\"",yytext);
#endif
	if(!rnode || !rnode->tty) return 0;
	if(!fnmatch(yytext,rnode->tty,FNM_NOESCAPE|FNM_PATHNAME)) 
		return 1;
	else return 0;
}

int yyparse();
 
int flagexp(char *expr)
{
	time_t tt;
	char *p;

#ifdef Y_DEBUG
	log("check expression \"%s\"",expr);
#endif
	time(&tt);now=localtime(&tt);
	p=strdup(expr);
	yyPTR=p;
#ifdef FLEX_SCANNER  /* flex requires reinitialization */
	yy_init=1;
/*  	yydebug=1; */
#endif
	flxpres=0;
	if(yyparse()) {
#ifdef Y_DEBUG
 		log("could not parse expression \"%s\", assume `false'",expr); 
#endif
		free(p);
		return -1;
	}
#ifdef Y_DEBUG
	log("checking result is \"%s\"",flxpres?"true":"false");
#endif
	free(p);
	return flxpres;
}

static int yyerror(char *s)
{
#ifdef Y_DEBUG
	log("parser error: %s",s);
#endif
	return 0;
}
