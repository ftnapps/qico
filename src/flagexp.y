/**********************************************************
 * expression parser
 * $Id: flagexp.y,v 1.14 2004/06/05 06:49:13 sisoft Exp $
 **********************************************************/
%token DATE DATESTR GAPSTR ITIME NUMBER PHSTR TIMESTR ADDRSTR IDENT 
%token CONNSTR SPEED CONNECT PHONE MAILER TIME ADDRESS FLEXEC FLLINE
%token DOW ANY WK WE SUN MON TUE WED THU FRI SAT EQ NE
%token GT GE LT LE LB RB AND OR NOT XOR COMMA ASTERISK
%token AROP LOGOP PORT CID FLFILE PATHSTR HOST SFREE ANYSTR
%expect 2
%{
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
		| SFREE AROP NUMBER PATHSTR
			{$$ = checksfree($2,$3);}
		| CONNSTR CONNSTR
			{$$ = checkconnstr();}
		| PHONE PHSTR
			{$$ = checkphone();}
		| MAILER IDENT
			{$$ = checkmailer();}
		| CID PHSTR
			{$$ = checkcid();}
		| HOST IDENT
			{$$ = checkhost();}
		| PORT IDENT
			{$$ = checkport();}
		| FLEXEC ANYSTR
			{$$ = checkexec();}
		| FLFILE PATHSTR
			{$$ = checkfile();}
		| FLLINE NUMBER
			{$$ = checkline($2);}
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
