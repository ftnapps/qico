/**********************************************************
 * File: zrecv.c
 * Created at Fri Jul 16 18:06:30 1999 by pk // aaz@ruxy.org.ru
 * receive zmodem, based on code by Chuck Forsberg
 * $Id: zrecv.c,v 1.7 2000/11/26 13:17:35 lev Exp $
 **********************************************************/
#include "headers.h"
#include <utime.h>
#include "defs.h"
#include "zmodem.h"
#include "qipc.h"

#define N_ZFINS 1

int tryzhdrtype=ZRINIT;	/* Header type to send corresponding to Last rx close */
static long total_length = 0;           /*          its length */
static char will_skip = 0;              /* skipping trigger.
                                           if set - than we want to skip current
                                           file. */


int procheader(char *name, char *path)
{
	char *p;
	int a,b,c,d,e,f;
#ifdef Z_DEBUG
	write_log("prochdr");
#endif	
	sline("ZRecv %s", name);
	p=name+1+strlen(name);
	if(*p) sscanf(p, "%d %o %d %d %d %d", &a, &b, &c, &d, &e, &f);
        total_length = a;
        will_skip = 0;
	switch(rxopen(name, b, a, &rxfd)) {
	case FOP_SKIP:
		return ZSKIP;
	case FOP_SUSPEND:
		return ERROR;
	case FOP_CONT:
		rxpos=recvf.soff;
		fseek(rxfd, rxpos, SEEK_SET);
		break;
	case FOP_OK:
		rxpos=0;
	}
	return OK;
}

/*
 * Ack a ZFIN packet, let byegones be byegones
 */
void ackbibi()
{
	int n;

#ifdef Z_DEBUG	
	write_log("ackbibi");
#endif
	stohdr(0L);
	for (n=3; --n>=0; ) {
		PURGE();
		zshhdr(4,ZFIN, Txhdr);
		switch (GETCHAR(10)) {
		case ZPAD:
			zgethdr(Rxhdr);
			break;
		case 'O':
			GETCHAR(1);	/* Discard 2nd 'O' */
			return;
		case RCDO:
			return;
		case TIMEOUT:
		default:
			break;
		}
	}
}

int tryz()
{
	int c, n, zfins=0;
	int cmdzack1flg;

#ifdef Z_DEBUG	
	write_log("tryz");
#endif
	for (n=15; --n>=0; ) {
		/* Set buffer length (0) and capability flags */
		stohdr(0L);
#ifdef CANBREAK
		Txhdr[ZF0] = CANFC32|CANFDX|CANOVIO|CANBRK;
#else
		Txhdr[ZF0] = CANFC32|CANFDX|CANOVIO;
#endif
		if (Zctlesc)
			Txhdr[ZF0] |= TESCCTL;
		Txhdr[ZF0] |= CANRLE;
		Txhdr[ZF1] = CANVHDR;
		/* tryzhdrtype may == ZRINIT */
		zshhdr(4,tryzhdrtype, Txhdr);
		if (tryzhdrtype == ZSKIP)	/* Don't skip too far */
			tryzhdrtype = ZRINIT;	/* CAF 8-21-87 */
again:
		switch (c=zgethdr(Rxhdr)) {
		case ZRQINIT:
			if (Rxhdr[ZF3] & 0x80)
				Usevhdrs = 1;	/* we can var header */
			continue;
		case ZEOF:
			continue;
		case ERROR:
			return ERROR;
		case RCDO:
			return RCDO;
		case TIMEOUT:
			continue;
		case ZFILE:
/* 			zconv = Rxhdr[ZF0]; */
/* 			zmanag = Rxhdr[ZF1]; */
/* 			ztrans = Rxhdr[ZF2]; */
			if (Rxhdr[ZF3] & ZCANVHDR)
				Usevhdrs = TRUE;
			tryzhdrtype = ZRINIT;
			c = zrdata(rxbuf, ZMAXBLOCK);
			if (c == GOTCRCW)
				return ZFILE;
			zshhdr(4,ZNAK, Txhdr);
			goto again;
		case ZSINIT:
			Zctlesc = TESCCTL & Rxhdr[ZF0];
			if (zrdata(Attn, ZATTNLEN) == GOTCRCW) {
				stohdr(1L);
				zshhdr(4,ZACK, Txhdr);
				goto again;
			}
			zshhdr(4,ZNAK, Txhdr);
			goto again;
		case ZFREECNT:
			stohdr(2147483647);
			zshhdr(4,ZACK, Txhdr);
			goto again;
		case ZCOMMAND:
			cmdzack1flg = Rxhdr[ZF0];
			if (zrdata(rxbuf, ZMAXBLOCK) == GOTCRCW) {
				if (cmdzack1flg & ZCACK1)
					stohdr(0L);
				else {
					write_log("command %s requested - poslan na ;)",
						  rxbuf);						  
					stohdr(0L);
				}
				PURGE();	/* dump impatient questions */
				do {
					zshhdr(4,ZCOMPL, Txhdr);
				}
				while (++rxretries<20 && zgethdr(Rxhdr) != ZFIN);
				ackbibi();
				return ZCOMPL;
			}
			zshhdr(4,ZNAK, Txhdr); goto again;
		case ZCOMPL:
			goto again;
		default:
			continue;
		case ZFIN:
			/* TODO: не верить первому ZFIN? */
			if((++zfins)==N_ZFINS) {
				ackbibi(); return ZCOMPL;
			}
			goto again;
		case ZCAN:
			return ERROR;
		}
	}
	return 0;
}


/*
 * Putsec writes the n characters of buf to receive file rxfd.
 *  If not in binary mode, carriage returns, and all characters
 *  starting with CPMEOF are discarded.
 */
int putsec(buf, n)
char *buf;
int n;
{
	char *p,tmp[255];
        struct stat statf;
#ifdef Z_DEBUG	
	write_log("putsec");
#endif
	sprintf(tmp, "%s/tmp/%s", ccs, recvf.fname);
	if ( !will_skip && stat(tmp, &statf) && errno == ENOENT) {
		fflush (rxfd);
		if ( stat(tmp, &statf) && errno == ENOENT ) {
			will_skip = 1;
			return OK;
		}
	}
		
	if (n == 0)
		return OK;
	for (p=buf; --n>=0; )
		if(putc( *p++, rxfd)==EOF) return ERROR;
	return OK;
}

/*
 * Receive a file with ZMODEM protocol
 *  Assumes file name frame is in rxbuf
 */
int rzfile(char *path)
{
	int c, n;
#ifdef Z_DEBUG	
	write_log("rzfile");
#endif

	n = 20; rxpos = 0l;

	if((c=procheader(rxbuf, path))) return (tryzhdrtype = c);

	for (;;) {
                /* Skiping sequence itself. */
                if ( will_skip ) {
                        rxpos = total_length;
                        PUTSTR(Attn);
                }
		stohdr(rxpos);
		zshhdr(4,ZRPOS, Txhdr);
nxthdr:
		switch (c = zgethdr(Rxhdr)) {
		case RCDO:
			rxclose(&rxfd, FOP_ERROR);
			return RCDO;
		default:
			if ( --n < 0) {
				return ERROR;
			}
			continue;
		case ZCAN:
			return ERROR;
		case ZNAK:
			if ( --n < 0) {
				return ERROR;
			}
			continue;
		case TIMEOUT:
			if ( --n < 0) {
				return ERROR;
			}
			continue;
		case ZFILE:
			zrdata(rxbuf, ZMAXBLOCK);
			continue;
		case ZEOF:
			if (rclhdr(Rxhdr) != rxpos) {
				/*
				 * Ignore eof if it's at wrong place - force
				 *  a timeout because the eof might have gone
				 *  out before we sent our zrpos.
				 */
				rxretries = 0;
				PUTSTR(Attn);
				continue;
			}
			if (will_skip) rxclose (&rxfd, FOP_SKIP);
			else rxclose(&rxfd, FOP_OK);
			return c;
		case ERROR:	/* Too much garbage in header search error */
			if ( --n < 0) {
				return ERROR;
			}
			PUTSTR(Attn);
			continue;
		case ZSKIP:
			rxclose(&rxfd, FOP_SKIP);
			return c;
		case ZDATA:
			if (rclhdr(Rxhdr) != rxpos) {
				if ( --n < 0) {
					return ERROR;
				}
				PUTSTR(Attn);  continue;
			}
		  moredata:
			c = zrdata(rxbuf, ZMAXBLOCK);
			switch (c)
			{
			case ZCAN:
				return ERROR;
			case ERROR:	/* CRC error */
				if ( --n < 0) {
					return ERROR;
				}
				PUTSTR(Attn);
				continue;
			case TIMEOUT:
				if ( --n < 0) {
					return ERROR;
				}
				continue;
			}
			n = 20;
			if (putsec(rxbuf, Rxcount)==ERROR) return (ERROR);
			rxpos += Rxcount;
			recvf.foff=rxpos;
			check_cps();
			qpfrecv();
			switch (c) {
			case GOTCRCW:
				stohdr(rxpos);
				PUTCHAR(XON);
				zshhdr(4,ZACK, Txhdr);
			case GOTCRCE:
                                if (will_skip) continue;                        
				goto nxthdr;
			case GOTCRCQ:
				stohdr(rxpos);
				zshhdr(4,ZACK, Txhdr);
			case GOTCRCG:
                                if (will_skip) continue;                        
				goto moredata;
			}
		}
	}
}

/*
 * Receive 1 or more files with ZMODEM protocol
 */
int rzfiles(char *path)
{
	int c;

#ifdef Z_DEBUG	
	write_log("rzfiles");
#endif
	for (;;) {
		c = rzfile(path);
		switch (c) {
		case ZEOF:
		case ZSKIP:
			switch (tryz()) {				
			case ZCOMPL:
				return OK;
			default:
				return ERROR;
			case ZFILE:
				break;
			}
			continue;
		case ERROR:
			return ERROR;
		default:
			return c;
		}
	}
	/* NOTREACHED */
}

int zmodem_receive(char *pathname)
{
	int c;
	HASDATA(10);
	rxbuf=malloc(8193);
	if((c=tryz())) {
		if (c == ZCOMPL) {
			sfree(rxbuf);
			return OK;
		}
		if (c != ERROR)
			c = rzfiles(pathname);
	}
	sfree(rxbuf);
	return c;
}
