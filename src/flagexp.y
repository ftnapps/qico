/**********************************************************
 * expression parser
 * $Id: flagexp.y,v 1.20 2004/06/22 14:26:21 sisoft Exp $
 **********************************************************/
%{
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
static int checkflag();
static int checkconnstr();
static int checkspeed(int op,int speed,int real);
static int checksfree(int op,int sp);
static int checkmailer();
static int checkphone();
static int checkport();
static int checkcid();
static int checkhost();
static int checkfile();
static int checkexec();
static int checkline(int lnum);
static int yyerror(char *s);
static int yyerror(char *s);
int flxpres;
%}

%token DATESTR GAPSTR PHSTR TIMESTR ADDRSTR PATHSTR ANYSTR IDENT NUMBER
%token AROP LOGOP EQ NE GT GE LT LE AND OR NOT XOR LB RB COMMA
%token ADDRESS ITIME CONNSTR SPEED CONNECT PHONE MAILER CID
%token FLTIME FLDATE EXEC FLLINE PORT FLFILE HOST SFREE
%expect 2

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
		| EXEC ANYSTR
			{$$ = checkexec();}
		| FLFILE PATHSTR
			{$$ = checkfile();}
		| FLLINE NUMBER
			{$$ = checkline($2);}
		| ITIME timestring
			{$$ = $2;}
		| FLTIME gapstring
			{$$ = $2;}
		| FLDATE datestring
			{$$ = $2;}
		| ADDRESS ADDRSTR
			{$$ = $2;}
		;
flag		: IDENT
			{$$ = checkflag();}
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

static int checkflag()
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
		q=xstrdup(rnode->flags);p=strtok(q,",");
		while(p) {
			if(!strcasecmp(yytext,p)) {
				xfree(q);
				DEBUG(('Y',3,"checkflag: other: 1"));
				return 1;
			}
			p=strtok(NULL,",");
		}
		xfree(q);
	}
	DEBUG(('Y',3,"checkflag: other: 0"));
	return 0;
}


static int checkconnstr()
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

static int checkphone()
{
	DEBUG(('Y',2,"checkphone: \"%s\"",yytext));
	if(!rnode||!rnode->phone) return 0;
	DEBUG(('Y',3,"checkphone: \"%s\" <-> \"%s\"",yytext,rnode->phone));
	if(!strncasecmp(yytext,rnode->phone,strlen(yytext))) return 1;
	return 0;
}

static int checkmailer()
{
	DEBUG(('Y',2,"checkmailer: \"%s\"",yytext));
	if(!rnode||!rnode->mailer) return 0;
	DEBUG(('Y',3,"checkmailer: \"%s\" <-> \"%s\"",yytext,rnode->mailer));
	if(!strstr(rnode->mailer,yytext)) return 1;
	return 0;
}

static int checkcid()
{
	char *cid = getenv("CALLER_ID");
	if(!cid||strlen(cid)<4) cid = "none";
	DEBUG(('Y',2,"checkcid: \"%s\" <-> \"%s\"",yytext,cid));
	if(!strncasecmp(yytext,cid,strlen(yytext))) return 1;
	return 0;
}

static int checkhost()
{
	DEBUG(('Y',2,"checkhost: \"%s\"",yytext));
	if(!rnode || !rnode->host) return 0;
	DEBUG(('Y',3,"checkhost: \"%s\" <-> \"%s\"",yytext,rnode->host));
	if(!strncasecmp(yytext,rnode->host,strlen(yytext)))return 1;
	return 0;
}

static int checkport()
{
	DEBUG(('Y',2,"checkport: \"%s\"",yytext));
	if(!rnode || !rnode->tty) return 0;
	DEBUG(('Y',3,"checkport: \"%s\" <-> \"%s\"",yytext,rnode->tty));
	if(!fnmatch(yytext,rnode->tty,FNM_NOESCAPE|FNM_PATHNAME)) return 1;
	return 0;
}

static int checkfile()
{
	struct stat sb;
	DEBUG(('Y',2,"checkfile: \"%s\" -> %d",yytext,!stat(yytext,&sb)));
	if(!stat(yytext,&sb)) return 1;
	return 0;
}

static int checkexec()
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

int flagexp(slist_t *expr,int strict)
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
