/**********************************************************
 * lexical parser
 * $Id: flaglex.l,v 1.20 2004/06/14 20:29:32 sisoft Exp $
 **********************************************************/
%option noyywrap case-insensitive never-interactive
%{

#ifndef yywrap
#define yywrap() 1
#endif

#undef ECHO

#ifdef FLEX_SCANNER
#undef YY_INPUT
#define YY_NO_UNPUT
#define YY_INPUT(buf,result,max_size) \
	if ((yyPTR == NULL) || (*yyPTR == '\0')) \
		result=YY_NULL; \
	else { \
		yyBUFL=strlen(yyPTR); \
		if (yyBUFL > max_size) yyBUFL=max_size; \
		memcpy(buf,yyPTR,yyBUFL); \
		yyPTR += yyBUFL; \
		result=yyBUFL; \
	}

#else /* this is most probably native lex? */

#undef input
#undef output
#undef unput
#define input() (((yyPTR == NULL) || (*yyPTR == '\0'))?NULL:*yyPTR++)
#define output(c)
#define unput(c) (*--yyPTR = (c))

#endif

#define RETV(x) yylval=x;return x;
#define RETN(x,f) yylval=f;return x;

#include "flagexp.h"
#include "headers.h"
static int chkaddr(char*);
static int chktime(char*);
static int chkdate(char*);
static int chkgaps(char*);
extern int yylval;
char *yyPTR=NULL;
int yyBUFL;
%}

minute		[0-5][0-9]
ihour		([0-1][0-9]|2[0-3])
hour        	([0-1]?[0-9]|2[0-3])
day		([0-2]?[0-9]|3[01])
dow		(sun?|mon?|tue?|wed?|thu?|fri?|sat?|any|wk|we)
month           (jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec|any)
decimal		[1-9][0-9]*
numeric		[0-9][0-9]*
ident		[a-z][a-z0-9\-]*
pathstr		\/([^\/]+\/)*[^\/]+
timestr		{dow}({ihour}{minute}\-{ihour}{minute})?
gapstr      	(([0-7].)?{hour}:{minute}\-{hour}:{minute}|(([0-7].)?{hour}\-{hour})|([0-7].{hour}:{minute}\-[0-7].{hour}:{minute})|([0-7].{hour}\-[0-7].{hour})|cm|t[a-x][a-x])
addrstr		(({decimal}|\*):)?(({decimal}|\*)\/)?({numeric}|\*)(.({numeric}|\*))?(@({ident}|\*))?
datestr         {month}({day}-{day})?
%%
[ \t\n]+	;
{decimal}	RETN(NUMBER,atol(yytext));
{pathstr}	RETV(PATHSTR);
[0-9\-]+	RETV(PHSTR);
==?		RETN(AROP,EQ);
!=		RETN(AROP,NE);
\>		RETN(AROP,GT);
\>=		RETN(AROP,GE);
\<		RETN(AROP,LT);
\<=		RETN(AROP,LE);
or|\|		RETN(LOGOP,OR);
and|\&		RETN(LOGOP,AND);
xor|\^		RETN(LOGOP,XOR);
not?|\!		RETV(NOT);
\(		RETV(LB);
\)		RETV(RB);
[\,\;]		RETV(COMMA);
addr(ess)?	RETV(ADDRESS);
cid		RETV(CID);
connect		RETV(CONNECT);
connstr		RETV(CONNSTR);
exec		RETV(EXEC);
file		RETV(FLFILE);
host		RETV(HOST);
itime		RETV(ITIME);
line		RETV(FLLINE);
mailer		RETV(MAILER);
phone		RETV(PHONE);
port		RETV(PORT);
sfree		RETV(SFREE);
speed		RETV(SPEED);
time		RETV(FLTIME);
date		RETV(FLDATE);
{timestr}	RETN(TIMESTR,chktime(yytext));
{datestr}       RETN(DATESTR,chkdate(yytext));
{gapstr}	RETN(GAPSTR,chkgaps(yytext));
{addrstr}	RETN(ADDRSTR,chkaddr(yytext));
{ident}		RETV(IDENT);
[^ \t\n]+	RETV(ANYSTR);
%%

static int chkaddr(char *str)
{
	FTNADDR_T(addr);
	falist_t *i;

	DEBUG(('Y',2,"chkaddr: \"%s\"",str));
	if(!parseftnaddr(str, &addr, &DEFADDR, 1)) {
		DEBUG(('Y',1,"chkaddr: bad address"));
		return 0;
	}
	if(!rnode) return 0;

	if(!rnode->addrs) {
		DEBUG(('Y',3,"chkaddr: check %d:%d/%d.%d",ndefaddr.z,ndefaddr.n,ndefaddr.f,ndefaddr.p));
		if((addr.z<0 || ndefaddr.z==addr.z) &&
			   (addr.n<0 || ndefaddr.n==addr.n) &&
			   (addr.f<0 || ndefaddr.f==addr.f) &&
			   (addr.p<0 || ndefaddr.p==addr.p)) {
			DEBUG(('Y',3,"chkaddr: 1"));
			return 1;
		}
	} else for(i=rnode->addrs;i;i=i->next) {
		DEBUG(('Y',3,"chkaddr: check %d:%d/%d.%d",i->addr.z,i->addr.n,i->addr.f,i->addr.p));
		if((addr.z<0 || i->addr.z==addr.z) &&
			   (addr.n<0 || i->addr.n==addr.n) &&
			   (addr.f<0 || i->addr.f==addr.f) &&
			   (addr.p<0 || i->addr.p==addr.p)) {
			DEBUG(('Y',3,"chkaddr: 1"));
			return 1;
		}
	}
	DEBUG(('Y',3,"chkaddr: 0"));
	return 0;
}

static int chkgaps(char *str)
{
	int r = checktimegaps(str);
	DEBUG(('Y',2,"chkgaps: \"%s\": %d",str,r));
	return r;
}

static int chktime(char *str)
{
	int h1,h2,m1,m2,beg,end,cur,dayok,day,r;
	struct tm *now;
	time_t tt;
	time(&tt);now=localtime(&tt);

	if (strncasecmp(str,"Su",2) == 0) day=0;
	else if (strncasecmp(str,"Mo",2) == 0) day=1;
	else if (strncasecmp(str,"Tu",2) == 0) day=2;
	else if (strncasecmp(str,"We",2) == 0) day=3;
	else if (strncasecmp(str,"Th",2) == 0) day=4;
	else if (strncasecmp(str,"Fr",2) == 0) day=5;
	else if (strncasecmp(str,"Sa",2) == 0) day=6;
	else if (strncasecmp(str,"Any",3) == 0) day=-1;
	else if (strncasecmp(str,"Wk",2) == 0) day=-2;
	else if (strncasecmp(str,"We",2) == 0) day=-3;
	else day=-4;

	DEBUG(('Y',2,"chktime: \"%s\": day %d, now %d",str,day,now->tm_wday));

	if (day >= 0) {
		dayok=(now->tm_wday == day);
	} else {
		switch (day)
		{
		case -3: dayok=((now->tm_wday == 0) || (now->tm_wday == 6)); break;
		case -2: dayok=((now->tm_wday != 0) && (now->tm_wday != 6)); break;
		case -1: dayok=1; break;
		default:
				DEBUG(('Y',1,"chktime: bad day %d",day));
				dayok=0;
				break;
		}
	}
	if (dayok == 0) return 0;

	while (*str && ((*str < '0') || (*str > '9'))) str++;
	if (*str == '\0') {
		DEBUG(('Y',3,"chktime: nothing after day: 1"));
		return 1;
	}
	if (sscanf(str,"%02d%02d-%02d%02d",&h1,&m1,&h2,&m2) != 4)
	{
		DEBUG(('Y',1,"chktime: invalid time \"%s\"",str));
		return 0;
	}
	cur=now->tm_hour*60+now->tm_min;
	beg=h1*60+m1;
	end=h2*60+m2;
	if (end > beg) r = ((cur >= beg) && (cur <= end));
	else r = ((cur >= beg) || (cur <= end));

	DEBUG(('Y',3,"chktime: %02d:%02d <= %02d:%02d <= %02d:%02d: %d",h1,m1,now->tm_hour,now->tm_min,h2,m2,r));
	return r;
}

static int chkdate(char *str)
{
	int d1,d2,m;
	char *p;
	struct tm *now;
	time_t tt;
	time(&tt);now=localtime(&tt);
	for(m=0;m<13;m++) if(!strncasecmp(str,engms[m],3)) break;
	DEBUG(('Y',2,"chkdate: \"%s\": month is %d, now %d",str,m,now->tm_mon));
	if(m>12 || (m!=12 && m!=now->tm_mon)) return 0;

	str+=3;while(*str==' '||*str=='\t') str++;
	if(!(p=strchr(str,'-'))) {
		if(sscanf(str,"%d", &d1)!=1) {
			DEBUG(('Y',1,"chkdate: no day: 0"));
			return 0;
		}
		d2=d1;
	} else if(sscanf(str,"%d-%d", &d1, &d2)!=1) {
		DEBUG(('Y',1,"chkdate: no two days: 0"));
		return 0;
	}
	DEBUG(('Y',3,"chkdate: days: %d-%d",d1,d2));
	if(d1>d2) {
		DEBUG(('Y',1,"chkdate: %d > %d",d1,d2));
		return 0;
	}
	DEBUG(('Y',3,"chkdate: %d",now->tm_yday>=d1 && now->tm_yday<=d2));
	return now->tm_yday>=d1 && now->tm_yday<=d2;
}
