/**********************************************************
 * File: flagexp.y
 * Created at Thu Jul 15 16:14:46 1999 by pk // aaz@ruxy.org.ru
 * Base version of this file was taken from Eugene Crosser's ifcico 
 * $Id: flagexp.y,v 1.9.4.1 2003/01/24 18:51:22 cyrilm Exp $
 **********************************************************/
%token DATE DATESTR GAPSTR ITIME NUMBER PHSTR TIMESTR ADDRSTR IDENT SPEED CONNECT PHONE TIME ADDRESS DOW ANY WK WE SUN MON TUE WED THU FRI SAT EQ NE GT GE LT LE LB RB AND OR NOT XOR COMMA ASTERISK AROP LOGOP PORT CID FLFILE PATHSTR
%{
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
#include <fnmatch.h>
#include "headers.h"

#define YY_NO_UNPUT
#undef ECHO
	
int flxpres;
	

static int logic(int e1, int op,int e2);
static int checkspeed(int op, int speed, int real);
static int checkphone(void);
static int checkport(void);
static int checkcid(void);
static int checkfile(void);
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
		| CID PHSTR
			{$$ = checkcid();}
		| PORT IDENT
			{$$ = checkport();}
		| FLFILE PATHSTR
			{$$ = checkfile();}
		| ITIME timestring
			{$$ = $2;}
		| TIME gapstring
			{$$ = $2;}
		| DATE datestring
			{$$ = $2;}
		| ADDRESS ADDRSTR
			{$$ = $2;}
		;
flag		:IDENT
		{$$ = $1;}
		;
datestring	: DATESTR
			{$$ = $1;}
		| DATESTR COMMA datestring
			{$$ = logic($1,OR,$3);}
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
		;
%%

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

static int checkspeed(int op, int speed, int real)
{
	DEBUG(('Y',2,"check%sspeed: %d (%d,%s) %d",real?"real":"",real?rnode->realspeed:rnode->speed,op,
		(EQ==op?"==":
		(NE==op?"!=":
        (GT==op?">":
        (GE==op?">=":
        (LT==op?"<":
        (LE==op?"<=":"???"
		)))))),speed));
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
		DEBUG(('Y',1,"Logic: invalid comparsion operator %d",op));
		return 0;
	}
}

static int checkphone(void)
{
	DEBUG(('Y',2,"checkphone: \"%s\"",yytext));
	if(!rnode) return 0;
	if(!rnode->phone) return 0;
	DEBUG(('Y',2,"checkphone: \"%s\" <-> \"%s\"",yytext,rnode->phone));
	if(!strncasecmp(yytext,rnode->phone,strlen(yytext))) return 1;
	return 0;
}

static int checkcid(void)
{
	char *cid = getenv("CALLER_ID");
	if(!cid) cid = "none";
	DEBUG(('Y',2,"checkcid: \"%s\" <-> \"%s\"",yytext,cid));
	if(!strncasecmp(yytext,cid,strlen(yytext))) return 1;
	return 0;
}

static int checkport(void)
{
	DEBUG(('Y',2,"checkport: \"%s\"",yytext));
	if(!rnode || !rnode->tty) return 0;
	DEBUG(('Y',2,"checkport: \"%s\" <-> \"%s\"",yytext,rnode->tty));
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


int yyparse();
 
int flagexp(char *expr)
{
	char *p;

	DEBUG(('Y',1,"checkexpression: \"%s\"",expr));

	p=strdup(expr);
#ifdef FLEX_SCANNER  /* flex requires reinitialization */
	yy_init=1;
#endif
	flxpres=0;
	if(yyparse()) {
		DEBUG(('Y',1,"checkexpression: could not parse, assume \"false\""));
		free(p);
		return 0;
	}
	DEBUG(('Y',1,"checkexpression: result is \"%s\"",flxpres?"true":"false"));
	free(p);
	return flxpres;
}

static int yyerror(char *s)
{
	DEBUG(('Y',1,"parser error: \"%s\"",s));
	return 0;
}
