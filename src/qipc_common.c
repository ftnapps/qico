/**********************************************************
 * File: qipc_common.c
 * Created at Mon May 7 23:07:41 2001  by lev // lev@serebryakov.spb.ru
 * 
 * $Id: qipc_common.c,v 1.5 2001/07/24 14:11:26 lev Exp $
 **********************************************************/
#include "headers.h"
#include <stdarg.h>
#include "byteop.h"

/* Decode packet by signature */
int unpack_ipc_packet(CHAR *data, int *len, char *sig, ...)
{
	va_list args;
	DWORD *pl;
	CHAR *pc;
	CHAR *ps;
	CHAR **pps;
	ftnaddr_t *pa;
	ninfo_t *pn;
	slist_t **ppsl, *psl;
    faslist_t **ppasl, *pasl;
	falist_t **ppal, *pal;
	long l,i,N;
	int rc = 0;

	va_start(args, sig);

	while(*sig) {
		switch(*sig) {
		case 's':		/* Strings, allocate here */
			if(*len<4) { va_end(args); return rc; }
			l=FETCH32(data); INC32(data); *len-=4;
			pps=va_arg(args,CHAR **);
			if(l) {
				if(*len<l || data[l-1]) { va_end(args); return rc; }
				*pps=xmalloc(l);
				memcpy(*pps,data,l); *len-=l; data+=l;
			} else {
				*pps=NULL;
			}
			rc++;
			break;
		case 'd':		/* 32 bit unsigned integer */
			if(*len<4) { va_end(args); return rc; }
			pl=va_arg(args, DWORD *);
			*pl=FETCH32(data); INC32(data); *len-=4;
			rc++;
			break;
		case 'c':		/* One character */
			if(!*len) { va_end(args); return rc; }
			pc=va_arg(args,CHAR *);
			*pc=*data; data++; *len--;
			rc++;
			break;
		case 'a':		/* 4D FTN address */
			if(*len<2*4) { va_end(args); return rc; }
			pa=va_arg(args,ftnaddr_t *);
			pa->z=FETCH16(data); INC16(data); *len-=2;
			pa->n=FETCH16(data); INC16(data); *len-=2;
			pa->f=FETCH16(data); INC16(data); *len-=2;
			pa->p=FETCH16(data); INC16(data); *len-=2;
			rc++;
			break;
		case 'l':		/* List of strings or addresses */
			if(*len<2) { va_end(args); return rc; }
			N=FETCH16(data); INC16(data); *len-=2;
			sig++;
			switch(*sig) {
			case 's':	/* List of strings (slist_t) */
				if(*len<4*N) { va_end(args); return rc; }
				ppsl=va_arg(args,slist_t **);
				psl=*ppsl=NULL;
				for(i=0;i<N;i++) {
					if(!psl) psl=*ppsl=xcalloc(1,sizeof(*psl));
					else { psl->next=xcalloc(1,sizeof(*psl)); psl=psl->next; }
					l=*len;
					if(1!=unpack_ipc_packet(data,len,"s",&psl->str)) { va_end(args); return rc; }
					data+=(l-*len);
				}
				break;
			case 'a':	/* List of addresses (falist_t) */
				if(*len<8*N) { va_end(args); return rc; }
				ppal=va_arg(args,falist_t **);
				pal=*ppal=NULL;
				for(i=0;i<N;i++) {
					if(!pal) pal=*ppal=xcalloc(1,sizeof(*pal));
					else { pal->next=xcalloc(1,sizeof(*pal)); pal=pal->next; }
					if(1!=unpack_ipc_packet(data,len,"a",&pal->addr)) { va_end(args); return rc; }
					data+=8;
				}
				break;
			default:
				va_end(args);
				return rc;
			}
			rc++;
			break;
		case 'n':	/* Node info "lassssdsds" */
			pn=va_arg(args,ninfo_t *);
			l=*len;
			if(9!=unpack_ipc_packet(data,len,"lassssdsds",
					&pn->addrs,
					&pn->name,
					&pn->place,
					&pn->sysop,
					&pn->phone,
					&pn->speed,
					&pn->flags,
					&pn->time,
					&pn->wtime)) { va_end(args); return rc; }
			data+=(l-*len);
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
	CHAR pc;
	CHAR *ps;
	ftnaddr_t *pa;
	long l;
	int slen = maxlen;
	int rc = 0;

	*len=0;
	va_start(args, sig);
	while(*sig) {
		switch(*sig) {
		case 's':		/* String */
			if(maxlen<4) { va_end(args); return rc; }
			ps=va_arg(args,CHAR *);
			if(ps) l=strlen(ps)+1;
			else l=0;
			STORE32(data,l); INC32(data); maxlen-=4;
			if(l) {
				if(maxlen<l) { va_end(args); return rc; }
				memcpy(data,ps,l); data+=l; maxlen-=l;
			}
			rc++;
			break;
		case 'd':		/* 32bit unsigned integer */
			if(maxlen<4) { va_end(args); return rc; }
			pl=va_arg(args, DWORD);
			STORE32(data,pl); INC32(data); maxlen-=4;
			rc++;
			break;
		case 'c':		/* One character */
			if(!maxlen) { va_end(args); return rc; }
			pc=va_arg(args,CHAR);
			*data=pc; data++; maxlen--;
			rc++;
			break;
		case 'a':		/* 4D FTN address */
			if(maxlen<8) { va_end(args); return rc; }
			pa=va_arg(args,ftnaddr_t);
			STORE16(data,pa->z); INC32(data); maxlen-=2;
			STORE16(data,pa->n); INC32(data); maxlen-=2;
			STORE16(data,pa->f); INC32(data); maxlen-=2;
			STORE16(data,pa->p); INC32(data); maxlen-=2;
			rc++;
			break;
		case 'l':		/* List of strings or addresses */
			if(maxlen<2) { va_end(args); return rc; }
			*ps=data;
			N=0;
			STORE16(data,N); INC16(data); *len-=2;
			sig++;
			switch(*sig) {
			case 's':	/* List of strings (slist_t) */
				if(maxlen<4*N) { va_end(args); return rc; }
				psl=va_arg(args,slist_t *);
				if(psl) do {
					N++;
					if(1!=pack_ipc_packet(data,len,&l,"s",psl->str)) { va_end(args); return rc; }
					data+=l;
				} while(psl=psl->next);
				break;
			case 'a':	/* List of addresses (falist_t) */
				if(maxlen<4*N) { va_end(args); return rc; }
				pal=va_arg(args,falist_t *);
				if(pal) do {
					N++;
					if(1!=pack_ipc_packet(data,len,&l,"a",&pal->addr)) { va_end(args); return rc; }
					data+=l;
				} while(pal=pal->next);
				break;
			default:
				va_end(args);
				return rc;
			}
			STORE16(ps,(N&0x0000ffff));	/* Store real number of elements back */
			rc++;
			break;
		case 'n':		/* Node info */
			pn=va_arg(args,ninfo_t *);
			if(9!=pack_ipc_packet(data,len,&l,"lassssdsds",
					pn->addrs,
					pn->name,
					pn->place,
					pn->sysop,
					pn->phone,
					pn->speed,
					pn->flags,
					pn->time,
					pn->wtime)) { va_end(args); return rc; }
			data+=l;
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

/* Encode packet with DES (pkt should be bigger than length!) */
void encode_ipc_packet(evtany_t *pkt, sessenccontext_t *cx)
{
	int len8 = ((pkg->fulllength>>3)|(pkg->fulllength&0x07?1:0))<<3;
	int i;
	for(i=pkg->fulllength;i<len8;i++) pkt->data[i] = 0;
	pkg->fulllength = len8;
	des_cbc_encrypt(cx->cx,cx->iv,pkg->data,pkg->data,len8);
}

/* Decode packet with DES (pkt should be bigger than length!) */
void decode_ipc_packet(evtany_t *pkt, sessenccontext_t *cx)
{
	int len8 = ((pkg->fulllength>>3)|(pkg->fulllength&0x07?1:0))<<3;
	int i;
	for(i=pkg->fulllength;i<len8;i++) pkt->data[i] = 0;
	pkg->fulllength = len8;
	des_cbc_decrypt(cx->cx,cx->iv,pkg->data,pkg->data,len8);
}
