/**********************************************************
 * File: qipc_common.c
 * Created at Mon May 7 23:07:41 2001  by lev // lev@serebryakov.spb.ru
 * 
 * $Id: qipc_common.c,v 1.2 2001/05/31 09:48:58 lev Exp $
 **********************************************************/
#include "headers.h"
#include <stdarg.h>
#include "byteop.h"

/* Decode packet by signature */
int unpack_ipc_packet(CHAR *data, int len, char *sig, ...)
{
	va_list args;
	DWORD *pl;
	char *pc;
	char *ps;
	ftnaddr_t *pa;
	long l;
	int rc = 0;

	va_start(args, sig);

	while(*sig) {
		switch(*sig) {
		case 's':		/* String */
			if(len<5) { va_end(args); return rc; }
			l=FETCH32(data); INC32(data); len-=4;
			if(len<l || data[l]) { va_end(args); return rc; }
			ps=va_arg(args,char *);
			memcpy(ps,data,l); len-=l; data+=l;
			rc++;
			break;
		case 'd':
			if(len<4) { va_end(args); return rc; }
			pl=va_arg(args, DWORD *);
			*pl=FETCH32(data); INC32(data); len-=4;
			rc++;
			break;
		case 'c':
			if(!len) { va_end(args); return rc; }
			pc=va_arg(args,char *);
			*pc=*data; data++; len--;
			rc++;
			break;
		case 'a':
			if(len<2*4) { va_end(args); return rc; }
			pa=va_arg(args,ftnaddr_t *);
			pa->z=FETCH16(data); INC16(data); len-=2;
			pa->n=FETCH16(data); INC16(data); len-=2;
			pa->f=FETCH16(data); INC16(data); len-=2;
			pa->p=FETCH16(data); INC16(data); len-=2;
			rc++;
			break;
		default:
			va_end(args);
			return rc;
		}
		sig++;
	}
	va_end(args);
	return rc;
}

/* Encode packet by signature */
int pack_ipc_packet(CHAR *data, int maxlen, int *len, char *sig, ...)
{
	va_list args;
	DWORD pl;
	char pc;
	char *ps;
	ftnaddr_t *pa;
	long l;
	int slen = maxlen;
	int rc = 0;

	*len=0;
	va_start(args, sig);
	while(*sig) {
		switch(*sig) {
		case 's':		/* String */
			if(maxlen<5) { va_end(args); return rc; }
			ps=va_arg(args,char *);
			l=strlen(ps)+1;
			STORE32(data,l); INC32(data); maxlen-=4;
			if(maxlen<l) { va_end(args); return rc; }
			memcpy(data,ps,l); maxlen-=l; data+=l;
			rc++;
			break;
		case 'd':
			if(maxlen<4) { va_end(args); return rc; }
			pl=va_arg(args, DWORD);
			STORE32(data,pl); INC32(data); maxlen-=4;
			rc++;
			break;
		case 'c':
			if(!maxlen) { va_end(args); return rc; }
			pc=va_arg(args,char);
			*data=pc; data++; maxlen--;
			rc++;
			break;
		case 'a':
			if(maxlen<2*4) { va_end(args); return rc; }
			pa=va_arg(args,ftnaddr_t);
			STORE16(data,pa->z); INC32(data); maxlen-=2;
			STORE16(data,pa->n); INC32(data); maxlen-=2;
			STORE16(data,pa->f); INC32(data); maxlen-=2;
			STORE16(data,pa->p); INC32(data); maxlen-=2;
			rc++;
			break;
		default:
			va_end(args);
			return rc;
		}
		sig++;
	}
	va_end(args);
	*len=slen-maxlen;
	return rc;
}
