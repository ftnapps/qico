/**********************************************************
 * File: zsend.c
 * Created at Fri Jul 16 18:06:30 1999 by pk // aaz@ruxy.org.ru
 * send zmodem, based on code by Chuck Forsberg
 * $Id: zsend.c,v 1.7 2000/11/26 13:17:35 lev Exp $
 **********************************************************/
#include "headers.h"
#include "defs.h"
#include "zmodem.h"
#include "qipc.h"


char Myattn[]={0};
int Lztrans, Lrxpos, bytcnt, Beenhereb4, Txwcnt;
unsigned Rxbuflen=16384, Txwspac, Tframlen;


/* Send send-init information */
int sendzsinit()
{
	int c;

#ifdef Z_DEBUG	
	write_log("sendzsinit");
#endif
	if (Myattn[0] == '\0' && (!Zctlesc || (rxoptions & TESCCTL)))
		return OK;
	txretries = 0;
	for (;;) {
		stohdr(0L);
#ifdef ALTCANOFF
		Txhdr[ALTCOFF] = ALTCANOFF;
#endif
		if (Zctlesc) {
			Txhdr[ZF0] |= TESCCTL; zshhdr(4, ZSINIT, Txhdr);
		}
		else
			zsbhdr(4, ZSINIT, Txhdr);
		zsdata(Myattn, ZATTNLEN, ZCRCW);
		c = zgethdr(Rxhdr);
		switch (c) {
		case RCDO:
			return RCDO;
		case ZCAN:
			return ERROR;
		case ZACK:
			return OK;
		default:
			if (++txretries > 19) return ERROR;
		}
	}
}

/*
 * Get the receiver's init parameters
 */
int getzrxinit(int canzap)
{
	int n;

#ifdef Z_DEBUG	
	write_log("getzrxinit");
#endif
	for (n=10; --n>=0; ) {
		
		switch (zgethdr(Rxhdr)) {
		case ZCHALLENGE:	/* Echo receiver's challenge numbr */
			stohdr(rxpos);
			zshhdr(4, ZACK, Txhdr);
			continue;
		case ZCOMMAND:		/* They didn't see out ZRQINIT */
			stohdr(0L);
			zshhdr(4, ZRQINIT, Txhdr);
			continue;
		case ZRINIT:
			rxoptions = 0377 & Rxhdr[ZF0];
			Usevhdrs = Rxhdr[ZF1] & CANVHDR;
			Txfcs32 = (rxoptions & CANFC32);
			Zctlesc |= rxoptions & TESCCTL;
			Rxbuflen = (0377 & Rxhdr[ZP0])+((0377 & Rxhdr[ZP1])<<8);
			if ( !(rxoptions & CANFDX))
				txwindow = 0;

			/* Override to force shorter frame length */
			if (Rxbuflen && (Rxbuflen>Tframlen) && (Tframlen>=32))
				Rxbuflen = Tframlen;
			if ( !Rxbuflen && (Tframlen>=32) && (Tframlen<=ZMAXBLOCK))
				Rxbuflen = Tframlen;

			/* Set initial subpacket length */
			if (txblklen < ZMAXBLOCK) {	/* Command line override? */
				if (effbaud >= 300)
					txblklen = 256;
				if (effbaud >= 1200)
					txblklen = 512;
				if(canzap) {
					if (effbaud >= 2400)
						txblklen = 2048;
					if (effbaud >= 9600)
						txblklen = 8192;
				} else {
					if (effbaud >= 2400)
						txblklen = 1024;
				}
			}
			if (Rxbuflen && txblklen>Rxbuflen)
				txblklen = Rxbuflen;

			if (Lztrans == ZTRLE && (rxoptions & CANRLE))
				Txfcs32 = 2;
			else
				Lztrans = 0;

			return (sendzsinit());
		case ZCAN:
		case RCDO:
			return RCDO;
		case TIMEOUT:
			return ERROR;
		case ZRQINIT:
			if (Rxhdr[ZF0] == ZCOMMAND)
				continue;
		default:
			zshhdr(4, ZNAK, Txhdr);
			continue;
		}
	}
	return ERROR;
}

/*
 * Respond to receiver's complaint, get back in sync with receiver
 */
int getinsync(flag)
{
	int c=OK;
#ifdef Z_DEBUG	
	write_log("getinsync");
#endif
	for (;;) {
		c = zgethdr(Rxhdr);
		switch (c) {
		case RCDO:
		case ERROR:
			return RCDO;
		case ZCAN:
		case ZABORT:
		case ZFIN:
		case TIMEOUT:
			return ERROR;
		case ZRPOS:
			if (rxpos > bytcnt) {
				return ZRPOS;
			}
			/* ************************************* */
			/*  If sending to a buffered modem, you  */
			/*   might send a break at this point to */
			/*   dump the modem's buffer.		 */
			clearerr(txfd);	/* In case file EOF seen */
			if (fseek(txfd, rxpos, 0)) {
				return ERROR;
			}
			bytcnt = Lrxpos = txpos = rxpos;
			if (txsyncid == rxpos) {
				if (++Beenhereb4 > 12) {
					return ERROR;
				}
				if (Beenhereb4 > 4)
					if (txblklen > 32)
						txblklen /= 2;
			}
			else
				Beenhereb4 = 0;
			txsyncid = rxpos;
			return c;
		case ZACK:
			Lrxpos = rxpos;
			if (flag || txpos == rxpos)
				return ZACK;
			continue;
		case ZRINIT:
			return c;
		case ZSKIP:
			return c;
		default:
			zsbhdr(4, ZNAK, Txhdr);
			continue;
		}
	}
}

/* Send the data in the file */
int zsendfdata()
{
	int c=OK, e, n;
	int newcnt;
	long tcount = 0;
	int junkcount;		/* Counts garbage chars received by TX */
#ifdef Z_DEBUG	
	write_log("zsendfdata");
#endif
	junkcount = 0;
	Beenhereb4 = FALSE;
  somemore:
	goto qwer;
  waitack:
	junkcount = 0;
	c = getinsync(0);
  gotack:
	switch (c) {
	case RCDO:
		txclose(&txfd, FOP_ERROR);
		return RCDO;
	case ZSKIP:
	case ZRINIT:
		txclose(&txfd, FOP_SKIP);
		return ZSKIP;
	case ZACK:
	case ZRPOS:
		break;
	case ZCAN:
	default:
		txclose(&txfd, FOP_ERROR);
		return ERROR;
	}

  qwer:
	newcnt = Rxbuflen;
	Txwcnt = 0;
	stohdr(txpos);
	zsbhdr(4, ZDATA, Txhdr);

	do {
		n = fread(txbuf, 1, txblklen, txfd);
		if (n<txblklen)
			e = ZCRCE;
		else if (junkcount > 3)
			e = ZCRCW;
		else if (bytcnt == txsyncid)
			e = ZCRCW;
		else if (Rxbuflen && (newcnt -= n) <= 0)
			e = ZCRCW;
		else if (txwindow && (Txwcnt += n) >= Txwspac) {
			Txwcnt = 0;  e = ZCRCQ;
		} else
			e = ZCRCG;

		zsdata(txbuf, n, e);
		qpfsend();
		check_cps();

		bytcnt = txpos += n;
		if (e == ZCRCW)
			goto waitack;
		
		if (txwindow) {
			while ((tcount = (txpos - Lrxpos)) >= txwindow) {
				if (e != ZCRCQ)
					zsdata(txbuf, 0, e = ZCRCQ);
				c = getinsync(1);
				if (c != ZACK) {
					zsdata(txbuf, 0, ZCRCE);
					goto gotack;
				}
			}
		}
	} while (n>=txblklen);

	for (;;) {
		stohdr(txpos);
		zsbhdr(4, ZEOF, Txhdr);
	  egotack:
		switch (getinsync(0)) {
		case ERROR:
		case RCDO:
			txclose(&txfd, FOP_ERROR);
			return RCDO;
		case ZACK:
			goto egotack;
		case ZNAK:
			continue;
		case ZRPOS:
			goto somemore;
		case ZRINIT:
			sendf.foff=txpos;
			txclose(&txfd, FOP_OK);
			return OK;
		case ZSKIP:
			txclose(&txfd, FOP_SKIP);
			return c;
		default:
			txclose(&txfd, FOP_ERROR);
			return ERROR;
		}
	}
}

/* Send file name and related info */
int zsendfile(char *buf, int blen)
{
	int c;
	unsigned long crc=0;
	long lastcrcrq = -1;

#ifdef Z_DEBUG	
	write_log("zsendfile");
#endif
	
	for (txretries=0; ++txretries<11;) {
		Txhdr[ZF0] = 0; 	/* file conversion request */
		Txhdr[ZF1] = 0; /* file management request */
/* 		if (Lskipnocor) */
/* 			Txhdr[ZF1] |= ZMSKNOLOC; */
		Txhdr[ZF2] = Lztrans;	/* file transport request */
		Txhdr[ZF3] = 0;
		zsbhdr(4, ZFILE, Txhdr);
		zsdata(buf, blen, ZCRCW);
again:
		c = zgethdr(Rxhdr);
		switch (c) {
		case RCDO:
		case ERROR:
			return RCDO;
		case ZFIN:
			return ERROR;
		case ZRINIT:
			while ((c=GETCHAR(5))>0) if (c == ZPAD) goto again;
			continue;
		case ZCAN:
		case TIMEOUT:
			continue;
		case ZABORT:
		default:
			continue;
		case ZNAK:
			continue;
		case ZCRC:
			if (rxpos != lastcrcrq) {
				lastcrcrq = rxpos;
				crc = 0xFFFFFFFFL;
				fseek(txfd, 0L, 0);
				while (((c = getc(txfd)) != EOF) && --lastcrcrq)
					crc = UPDC32(c, crc);
				crc = ~crc;
				clearerr(txfd);	/* Clear possible EOF */
				lastcrcrq = rxpos;
			}
			stohdr(crc);
			zsbhdr(4, ZCRC, Txhdr);
			goto again;
		case ZFERR:
			txclose(&txfd, FOP_SUSPEND);
			return c;
		case ZSKIP:
			txclose(&txfd, FOP_SKIP);
			return c;
		case ZRPOS:
			/*
			 * Suppress zcrcw request otherwise triggered by
			 * lastyunc==bytcnt
			 */
			sendf.soff=rxpos;
			if(fseek(txfd, rxpos, 0))
				return ERROR;
			txsyncid = (bytcnt = txpos = Lrxpos = rxpos) -1;
			return zsendfdata();
		}
	}
	txclose(&txfd, FOP_ERROR);
	return ERROR;
}


int zmodem_sendinit(int canzap)
{
 	PUTSTR("rz\r"); 
	txbuf=malloc(canzap?8193:1025);
	stohdr(0L);
	txblklen=128;rxoptions=0;
	do zshhdr(4, ZRQINIT, Txhdr);
  	while(HASDATA(5)==TIMEOUT);
	return getzrxinit(canzap);
}

int zmodem_sendfile(char *tosend, char *sendas,
					unsigned long *totalleft, unsigned long *filesleft)
{
	byte *q;
	int rc;

	txfd=txopen(tosend, sendas);
	sline("ZSend %s %p", sendas); 
	if(txfd) {
		strcpy(txbuf, sendas);
		q=strchr(txbuf, 0)+1;
		sprintf(q, "%d %lo 0 0 %ld %ld",
				sendf.ftot, sendf.mtime,
				*filesleft, *totalleft);
		rc=zsendfile(txbuf, strlen(q)+(q-txbuf)+1);
		(*totalleft)-=sendf.ftot;(*filesleft)--;
		return rc;
	}
	sline("ZS: File not found %s!", tosend);
	return OK;
}

int zmodem_senddone()
{
#ifdef Z_DEBUG
	write_log("zsenddone");
#endif
	for (;;) {
		stohdr(0L);		/* CAF Was zsbhdr - minor change */
		zshhdr(4, ZFIN, Txhdr);	/*  to make debugging easier */
		switch (zgethdr(Rxhdr)) {
		case ZFIN:
			PUTCHAR('O');PUTCHAR('O');/* FLUSH(); */
		case ZCAN:
		case TIMEOUT:
			sfree(txbuf);
			return OK;
		case ERROR:
		case RCDO:
			sfree(txbuf);
			return RCDO;
		}
	}
}
