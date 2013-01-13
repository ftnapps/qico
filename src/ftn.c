/**********************************************************
 * ftn tools
 **********************************************************/
/*
 * $Id: ftn.c,v 1.10 2005/08/18 16:21:25 mitry Exp $
 *
 * $Log: ftn.c,v $
 * Revision 1.10  2005/08/18 16:21:25  mitry
 * Added debug messages
 *
 * Revision 1.9  2005/08/10 19:19:30  mitry
 * Changed strncasecmp() to strcasecmp() in whattype()
 *
 * Revision 1.8  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.7  2005/05/11 20:18:26  mitry
 * Cosmetic change
 *
 * Revision 1.6  2005/05/06 20:34:09  mitry
 * whattype() is slightly changed.
 * Misc code cleanup.
 *
 * Revision 1.5  2005/03/28 17:02:52  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 */

#include "headers.h"
#include <fnmatch.h>
#include "crc.h"

/* domain name to translate ftn address to inet host. */
#define FTNDOMAIN "fidonet.net"


void addr_cpy(ftnaddr_t *a, const ftnaddr_t *b)
{
	if ( !a || !b )
		return;
	a->z = b->z;
	a->n = b->n;
	a->f = b->f;
	a->p = b->p;
	if ( b->d && *b->d )
		a->d = xstrdup( b->d );
	else
		a->d = NULL;
}


int addr_cmp(const ftnaddr_t *a, const ftnaddr_t *b)
{
	if ( !a || !b )
		return 0;

	return ( a->z == b->z && a->n == b->n 
		&& a->f == b->f && a->p == b->p ) ? 1 : 0;
}


int parseftnaddr(const char *s, ftnaddr_t *a, const ftnaddr_t *b, int wc)
{
	const char	*p = s,	*pn;
	int		n = -1, wn = 0, pq = 1;

	if(!s||!*s)return 0;
	if(b) {
		addr_cpy(a,b);
		a->p=wc?-1:0;
	} else a->z=a->n=a->f=a->p=wc?-1:0,a->d=NULL;
	for(pn=p;*p&&pq;p++) {
		if(isdigit((int)*p))wn=1;
		else if(wc&&*p=='*')wn=2;
		else switch(*p) {
			case ':':
				if(!wn||n>0)return 0;
				a->z=(wn<2)?atoi(pn):-1;
				pn=p+1;wn=0;n=1;
				break;
			case '/':
				if(!wn||n>1)return 0;
				a->n=(wn<2)?atoi(pn):-1;
				n=2;pn=p+1;wn=0;
				break;
			case '.':
				if(n>2)return 0;
				if(wn)a->f=(wn<2)?atoi(pn):-1;
				n=3;pn=p+1;
				if(!wn&&n<0)break;
				if(!wn)return 0;
				wn=0;
				break;
			case '@':
				if(p[1]) {
					xfree(a->d);
					a->d=xstrdup(p+1);
				}
			case '\n': case '\r':
			case ' ': case '\t':
			case '\0':
				pq=0;
				break;
			default:return 0;
		}
	}
	switch(n) {
	    case 2:
		if(!wn)return 0;
		a->f=(wn<2)?atoi(pn):-1;
		break;
	    case 3:
		if(!wn)return 0;
		a->p=(wn<2)?atoi(pn):-1;
		break;
	    case -1:
		if(!wn)return 0;
		a->f=(wn<2)?atoi(pn):-1;
		break;
	    default:
		return 0;
	}
	return 1;
}


ftnaddr_t *akamatch(const ftnaddr_t *a, falist_t *akas)
{
	int m,bm=0;
	falist_t *i;
	ftnaddr_t *best=&akas->addr;
	for(i=akas;i;i=i->next) {
		m=0;
		if(i->addr.z==a->z) {
			m=1;
			if(i->addr.n==a->n) {
				m=2;
				if(i->addr.f==a->f) {
					m=3;
					if(i->addr.p==a->p)m=4;
				}
			}
		}
		if(m>bm) {
			bm=m;
			best=&i->addr;
		}
	}
	return best;
}

char *ftnaddrtoa(const ftnaddr_t *a)
{
	static char s[30];
	if(a->p)snprintf(s,30,"%d:%d/%d.%d",a->z,a->n,a->f,a->p);
	    else snprintf(s,30,"%d:%d/%d",a->z,a->n,a->f);
	return s;
}

char *ftnaddrtoda(const ftnaddr_t *a)
{
	char *d=cfgs(CFG_DOMAIN);
	static char s[64];
	if(!*d)d="fidonet.org";
	if(*d=='@')d++;
	if(a->p)snprintf(s,64,"%d:%d/%d.%d@%s",a->z,a->n,a->f,a->p,a->d?a->d:d);
	    else snprintf(s,64,"%d:%d/%d@%s",a->z,a->n,a->f,a->d?a->d:d);
	return s;
}

char *ftnaddrtoia(const ftnaddr_t *a)
{
	static char s[64];
	if(a->p)snprintf(s,64,"p%d.f%d.n%d.z%d." FTNDOMAIN,a->p,a->f,a->n,a->z);
	    else snprintf(s,64,"f%d.n%d.z%d." FTNDOMAIN,a->f,a->n,a->z);
	return s;
}

char *strip8(char *s)
{
	register int	t, i = 0;
	static char	buf[MAX_STRING + 5];

	if ( !s )
		return NULL;

	if ( *s )
		recode_to_remote( s );

	while( *s && i < MAX_STRING) {
		t = *s;
		if ( t == '}' || t == ']' ) {
			if ( !bink ) {
				buf[i++] = t;
				buf[i] = t;
			}
		} else if ( isprint( t ))
			buf[i] = t;
		else {
			buf[i++] = '\\';
			buf[i++] = hexdigitsupper[(t >> 4) & 0x0f];
			buf[i] = hexdigitsupper[t & 0x0f];
		}
		i++;
		s++;
	}

	buf[i] = '\0';
	return xstrdup( buf );
}


int has_addr(const ftnaddr_t *a, falist_t *l)
{
	while(l) {
		if(addr_cmp(a,&l->addr))return 1;
		l=l->next;
	}
	return 0;
}

int showpkt(const char *fn)
{
	FILE *f;
	int i,n=1;
	pkthdr_t ph;
	pktmhdr_t mh;
	char from[36],to[36],a;
	f=fopen(fn,"r");
	if(!f){write_log("can't open '%s' for reading: %s",fn,strerror(errno));return 0;}
	if(fread(&ph,sizeof(ph),1,f)!=1)write_log("packet read error");
	    else if(I2H16(ph.phType)!=2)write_log("packet isn't 2+ format");
		else {
		    while(fread(&mh,sizeof(mh),1,f)==1) {
			i=0;while(fgetc(f)>0&&i<30)i++;i=0;
			if(i>=30)break;
			while((a=fgetc(f))>0&&i<36)to[i++]=a;
			if(i>=36)break;
			to[i]=0;i=0;
			while((a=fgetc(f))>0&&i<36)from[i++]=a;
			if(i>=32)break;
			from[i]=0;i=0;
			while(fgetc(f)>0&&i<72)i++;
			if(i>=72)break;
			while(fgetc(f)>0);
			write_log("*msg: %d from: \"%s\", to: \"%s\"",n++,from,to);
		    }
	}
	fclose(f);
	return 0;
}

FILE *openpktmsg(const ftnaddr_t *fa, const ftnaddr_t *ta,
	char *from, char *to, char *subj, char *pwd, char *fn, unsigned attr)
{
	FILE *f;
	pkthdr_t ph;
	pktmhdr_t mh;
	time_t tim=time(NULL);
	struct tm *t=localtime(&tim);
	if(!(f=fopen(fn,"w")))return NULL;
	memset(&ph,0,sizeof(ph));
	memset(&mh,0,sizeof(mh));
	ph.phONode=H2I16(fa->f);
	ph.phDNode=H2I16(ta->f);
	ph.phONet=H2I16(fa->n);
	ph.phDNet=H2I16(ta->n);
	ph.phYear=H2I16(t->tm_year+1900);
	ph.phMonth=H2I16(t->tm_mon);
	ph.phDay=H2I16(t->tm_mday);
	ph.phHour=H2I16(t->tm_hour);
	ph.phMinute=H2I16(t->tm_min);
	ph.phSecond=H2I16(t->tm_sec);
	ph.phBaud=H2I16(0);
	ph.phPCode=0xFE;
	ph.phType=H2I16(2);
	ph.phAuxNet=H2I16(0);
	ph.phCWValidate=H2I16(0x100);
	ph.phPCodeHi=1;
	ph.phPRevMinor=2;
	ph.phCaps=H2I16(1);
	if(pwd)memcpy(ph.phPass,pwd,MAX(strlen(pwd),8));
	ph.phQOZone=H2I16(fa->z);
	ph.phQDZone=H2I16(ta->z);
	ph.phOZone=H2I16(fa->z);
	ph.phDZone=H2I16(ta->z);
	ph.phOPoint=H2I16(fa->p);
	ph.phDPoint=H2I16(ta->p);
	fwrite(&ph,sizeof(ph),1,f);
	mh.pmONode=H2I16(fa->f);
	mh.pmDNode=H2I16(ta->f);
	mh.pmONet=H2I16(fa->n);
	mh.pmDNet=H2I16(ta->n);
	mh.pmAttr=H2I16(attr);
	mh.pmType=H2I16(2);
	fwrite(&mh,sizeof(mh),1,f);
	fprintf(f,"%02d %3s %02d  %02d:%02d:%02d%c",t->tm_mday,engms[t->tm_mon],t->tm_year%100,t->tm_hour,t->tm_min,t->tm_sec,0);
	if(cfgi(CFG_RECODEPKTS)) {
		recode_to_remote(to);
		if(attr<128) {
			recode_to_remote(subj);
			recode_to_remote(from);
		}
	}
	fwrite(to,strlen(to)+1,1,f);
	fwrite(from,strlen(from)+1,1,f);
	fwrite(subj,strlen(subj)+1,1,f);
	fprintf(f,"\001INTL %d:%d/%d %d:%d/%d\r",ta->z,ta->n,ta->f,fa->z,fa->n,fa->f);
	if(fa->p)fprintf(f,"\001FMPT %d\r",fa->p);
	if(ta->p)fprintf(f,"\001TOPT %d\r",ta->p);
	fprintf(f,"\001MSGID: %s %08lx\r",ftnaddrtoa(fa),sequencer());
	fprintf(f,"\001PID: %s %s/%s\r",progname,version,osname);
	return f;
}

void closepkt(FILE *f, const ftnaddr_t *fa, const char *tear, char *orig)
{
	if(cfgi(CFG_RECODEPKTS))recode_to_remote(orig);
	fprintf(f,"--- %s\r * Origin: %s (%s)\r%c%c%c",tear,orig,ftnaddrtoa(fa),0,0,0);
	fclose(f);
}


void closeqpkt(FILE *f, const ftnaddr_t *fa)
{
	char str[MAX_STRING+1];

	snprintf( str, MAX_STRING, "%s-%s/%s", qver( 0 ), qver( 1 ), qver( 2 ));
	closepkt( f, fa, str, cfgs( CFG_STATION ));
}


int whattype(const char *fn)
{
	static char	*ext[] = {"su","mo","tu","we","th","fr","sa","pkt","req"};
	register int	i;
	char		*p, low;

	if ( !fn )
		return IS_ERR;

	p = strrchr( fn, '.' );
	if ( !p )
		return IS_FILE;

	if ( strlen( ++p ) != 3 )
		return IS_FILE;

	if ( strcasecmp( p + 1, "lo" ) == 0 )
		return IS_FLO;

	for( i = 0; i < 9; i++ )
		if ( !strncasecmp( p, ext[i], strlen( ext[i] )))
			switch( i ) {
			case 7: return IS_PKT;
			case 8: return IS_REQ;
			default:
				low = tolower( p[2] );
				if (( low >= '0' && low <= '9' )
					|| ( low >= 'a' && low <= 'z' ))
					return IS_ARC;
				break;
			}
	return IS_FILE;
}


int istic(const char *fn)
{
	size_t flen;

	if ( !fn || ( flen = strlen( fn )) < 4 )
		return 0;

	return ( strncasecmp( fn + flen - 4, ".tic", 4 ) == 0 );
}


int havestatus(int status, int cfgkey)
{
	static int stc[] = { Q_NORM, Q_HOLD, Q_DIR, Q_CRASH, Q_IMM, Q_REQ };
	static char stl[] = Q_CHARS;
	register int i;
	char *callon = cfgs( cfgkey );

	for( i = 0; i < F_MAX; i++ )
		if (( status & stc[i] ) && ( callon && strchr( callon, stl[i] ))) {
			DEBUG(('Q',1,"havestatus: yes, i %d, callon '%s', status 0x%X",i,callon,status));
			return 1;
		}
	DEBUG(('Q',1,"havestatus: no, callon '%s', status 0x%X",callon,status));
	return 0;
}


int needhold(int status, int what)
{
	status &= Q_ANYWAIT;
	if ( status & Q_WAITA ) return 1;
	if (( status & Q_WAITR ) && !( what & (~T_REQ))) return 1;
	if (( status & Q_WAITX ) && !( what & (~T_ARCMAIL)))return 1;
	return 0;
}


int xfnmatch(char *pat, const char *name, int flags)
{
	int type,rc=0,q=0;
	if(!name||!pat)return FNM_NOMATCH;
	type=whattype(name);
	if(*pat=='!'){pat++;rc=1;}
	if(*pat=='%') {
		switch(pat[1]) {
		    case 'F':
			if(type==IS_FILE)q=1;
			break;
		    case 'N':
			if(type==IS_PKT)q=1;
			break;
		    case 'E':
			if(type==IS_ARC)q=1;
			break;
		    default:
			write_log("error: mask '%s': unknown macro '%c'",pat-rc,pat[1]);
			return FNM_NOMATCH;
		}
	} else q=!fnmatch(pat,name,flags);
	DEBUG(('S',4,"xfnm: '%s', pat='%s'. type=%d, q=%d, rc=%d",name,pat,type,q,rc));
	return((rc^q)?0:FNM_NOMATCH);
}


char *findpwd(const ftnaddr_t *a)
{
	faslist_t *cf;

	for( cf = cfgfasl( CFG_PASSWORD ); cf; cf = cf->next )
		if ( addr_cmp( &cf->addr, a ))
			return cf->str;
	return NULL;
}
