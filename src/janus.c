/******************************************************************
 * Janus protocol implementation with:
 * - freqs support
 * - crc32 support 
 * $Id: janus.c,v 1.1.1.1 2003/07/12 21:26:45 sisoft Exp $
 ******************************************************************/
/*---------------------------------------------------------------------------*/
/*                    Opus Janus revision 0.22,  1- 9-88                     */
/*                                                                           */
/*                The Opus Computer-Based Conversation System                */
/*           (c) Copyright 1987, Rick Huebner, All Rights Reserved           */
/*---------------------------------------------------------------------------*/
#include "headers.h"
#include <stdarg.h>
#include "defs.h"
#include "janus.h"
#include "byteop.h"

long   brain_dead;       /* Time at which to give up on other computer     */
slist_t *reqs=NULL;
unsigned int caps=0;

void preparereqs(flist_t *l);

/*****************************************************************************/
/* Super-duper neato-whizbang full-duplex streaming ACKless batch file       */
/* transfer protocol for use in Opus-to-Opus mail sessions                   */
/*****************************************************************************/
int janus()
{
	byte   pkttype;          /* Type of packet last received                   */
	int     blklen;           /* Length of last data block sent                 */
	word   goodneeded;       /* # good blocks to send before upping txblklen   */
	word   goodblks;         /* Number of good blocks sent at this block size  */
	word   rpos_count;       /* Number of RPOS packets sent at this position   */
	long   xmit_retry;       /* Time to retransmit lost FNAMEPKT or EOF packet */
	long   lasttx=0;           /* Position within file of last data block we sent*/
	long   rpos_retry;       /* Time at which to retry RPOS packet             */
	long   rpos_sttime;      /* Time at which we started current RPOS sequence */
	long   last_rpostime;    /* Timetag of last RPOS which we performed        */
	long   last_blkpos=0;      /* File position of last
								  out-of-sequence BLKPKT   */
	flist_t *l;
	int rc=0;
	char *p;	

	int capslogged=0;

	/*-------------------------------------------------------------------------*/
	/* Initialize file transmission variables                                  */
	/*-------------------------------------------------------------------------*/
	last_rpostime=
		xmit_retry   = 0L;
	if (effbaud < 300) effbaud = 300;
	timeout = 40960/effbaud;
	if (timeout < 10) timeout = 10;
	brain_dead=t_set(120);
	txmaxblklen     = effbaud/300 * 128;
	if (txmaxblklen > BUFMAX) txmaxblklen = BUFMAX;
	txblklen     = txmaxblklen;
	goodblks     = 0;
	goodneeded   = 3;
	txstate      = XSENDFNAME;

	txbuf=xcalloc(txmaxblklen+8,1);
	rxbuf=xcalloc(txmaxblklen+8,1);

	rxbufmax = rxbuf+BUFMAX+8;
	
	l=fl;
	preparereqs(l);
	getfname(&l);
	
	sline("Janus session (%d block)", txblklen);
  
	/*-------------------------------------------------------------------------*/
	/* Initialize file reception variables                                     */
	/*-------------------------------------------------------------------------*/
	rxbufptr = NULL;
	rpos_retry = rpos_count = 0;
	rxstate = RRCVFNAME;

	/*-------------------------------------------------------------------------*/
	/* Send and/or receive stuff until we're done with both                    */
	/*-------------------------------------------------------------------------*/
	do {   /*  while (txstate || rxstate)  */

		/*-----------------------------------------------------------------------*/
		/* If nothing useful (i.e. sending or receiving good data block) has     */
		/* happened within the last 2 minutes, give up in disgust                */
		/*-----------------------------------------------------------------------*/
		if (t_exp(brain_dead)) {
			write_log("other end died");  /* "He's dead, Jim." */
			goto giveup;
		}

		/*-----------------------------------------------------------------------*/
		/* If we're tired of waiting for an ACK, try again                       */
		/*-----------------------------------------------------------------------*/
		if (xmit_retry) {
			if (t_exp(xmit_retry)) {
				sline("Timeout waiting for ACK - retry");
				xmit_retry = 0L;

				switch (txstate) {
				case XRCVFNACK:
					txstate = XSENDFNAME;
					break;
				case XRCVEOFACK:
					if(fseek(txfd, txpos=lasttx, SEEK_SET)) {
						write_log("seek error on %s", sendf.fname);
						goto giveup;
					}
					txstate = XSENDBLK;
					break;
				case XRCVFRNAKACK:
					txstate = XSENDFREQNAK;
					break;
				}
			}
		}

		/*-----------------------------------------------------------------------*/
		/* Transmit next part of file, if any                                    */
		/*-----------------------------------------------------------------------*/
		switch (txstate) {
		case XSENDBLK:
			lasttx = txpos;
			STORE32(txbuf,lasttx);
			blklen = fread(txbuf+4,  1, txblklen, txfd);
			if(blklen<0) {
				write_log("read error on %s", sendf.fname);
				goto giveup;
			}
			sendpkt(txbuf, 4+blklen, BLKPKT);
			txpos += blklen;
			sendf.foff=txpos;
			check_cps();
			qpfsend();

			if (txpos >= sendf.ftot || blklen < txblklen) {
				xmit_retry=t_set(timeout);
				txstate  = XRCVEOFACK;
			} else brain_dead=t_set(120);

			if (txblklen < txmaxblklen && ++goodblks > goodneeded) {
				txblklen <<= 1;
				goodblks = 0;
			}
			break;

		case XSENDFNAME:
			blklen = strchr( strchr((char *)txbuf,'\0')+1, '\0') - (char *)txbuf + 3;
			sendpkt(txbuf,blklen,FNAMEPKT);
			xmit_retry=t_set(timeout);
			txstate = XRCVFNACK;
			sendf.cps=1;
			qpfsend();
			break;
		case XSENDFREQNAK:
			sendpkt (NULL, 0, FREQNAKPKT);
			xmit_retry=t_set(timeout);
			txstate = XRCVFRNAKACK;
			break;
		}

		/*-----------------------------------------------------------------------*/
		/* Catch up on our reading; receive and handle all outstanding packets   */
		/*-----------------------------------------------------------------------*/
		while ((pkttype = rcvpkt())) {
			DEBUG(('J',1,"rcvpkt %d (%c) len=%d at txs=%d rxs=%d",
				pkttype, C0(pkttype),
				rxblklen,
				txstate, rxstate));
			switch (pkttype) {

				/*-------------------------------------------------------------------*/
				/* File data block or munged block                                   */
				/*-------------------------------------------------------------------*/
			case BADPKT:
			case BLKPKT:
				if (rxstate == RRCVBLK) {
					unsigned int ln = FETCH32(rxbuf);
					if (pkttype == BADPKT || ln != rxpos) {
						if (pkttype == BLKPKT) {
							if (ln < last_blkpos) rpos_count = 0;
							last_blkpos = ln;
						}
						if (t_exp(rpos_retry))  {
							if (rpos_count > 5) rpos_count = 0;
							if (++rpos_count == 1) time(&rpos_sttime);
							sline("Bad packet at %ld", rxpos);
							STORE32(rxbuf, rxpos);
							STORE32(rxbuf+4, rpos_sttime);
							sendpkt(rxbuf, 8, RPOSPKT);
							rpos_retry=t_set(timeout/2);
						}
					} else {
						brain_dead=t_set(120);
						last_blkpos = rxpos;
						rpos_retry = rpos_count = 0;
                    				rxblklen -= 4;
						if(fwrite(rxbuf+4, rxblklen, 1, rxfd)<0) {
							write_log("write error on %s", recvf.fname);
							goto giveup;
						}
						rxpos += rxblklen;
						recvf.foff=rxpos;
						check_cps();
						qpfrecv();

						if (rxpos >= recvf.ftot) {
							rxclose(&rxfd, FOP_OK);
							rxstate = RRCVFNAME;
						}
					}
				}
				if (rxstate == RRCVFNAME) sendpkt(NULL,0,EOFACKPKT);
				break;

				/*-------------------------------------------------------------------*/
				/* Name and other data for next file to receive                      */
				/*-------------------------------------------------------------------*/
			case FNAMEPKT:
				p=(char *)(rxbuf+strlen((char *)rxbuf)+1);
				if (rxblklen > strlen((char *)rxbuf)+strlen(p)+2
					&& !caps) {
					caps=*((char *)strchr(p,0)+1) & OUR_JCAPS;
					if (!capslogged) {
						write_log("janus link options: %dKB%s%s%s",
							txmaxblklen/1024,
							caps & JCAP_CRC32?",C32":"", 
							caps & JCAP_FREQ?",FRQ":"",
							caps & JCAP_CLEAR8?",CL8":""
							);
						capslogged=1;
					}
				}
				if(rxstate==RRCVFNAME) {
					if(!rxbuf[0]) {
						if(reqs && (caps&JCAP_FREQ)) {
							snprintf((char*)txbuf, 1024, "%s%c%c", reqs->str, 0, caps);
							write_log("sent janus freq: %s", txbuf);
							sendpkt((byte*)txbuf,strlen((char*)txbuf)+2,FREQPKT);
							reqs=reqs->next;
							break;
						} else {
							rxstate=RDONE;
							qpreset(0);
						}
					} else {
						sscanf(p, "%u %lo %*o",
							  &recvf.ftot, &recvf.mtime);
						switch(rxopen((char *)rxbuf, recvf.mtime, recvf.ftot, &rxfd)){
						case FOP_SUSPEND:
							goto breakout;
						case FOP_CONT:
						case FOP_OK:
							qpfrecv();
							rxstate=RRCVBLK;
							rxpos=recvf.foff;
							break;
						case FOP_SKIP:
							rxpos=-1;
							break;
						}
						recvf.cps=1;
					}
				}
				STORE32(txbuf, rxpos);
				txbuf[4]=caps&0xff;
				sendpkt((byte *)txbuf,5,FNACKPKT);
				if (!(txstate || rxstate)) goto breakout;
				break;

				/*-------------------------------------------------------------------*/
				/* ACK to filename packet we just sent                               */
				/*-------------------------------------------------------------------*/
			case FNACKPKT:
				if (txstate == XRCVFNACK) {
					xmit_retry = 0L;
					caps=(rxblklen>=5)?rxbuf[4]:0;
					if(txfd) {
						if ((txpos = FETCH32(rxbuf)) > -1L) {
							sendf.soff=txpos;
							if(fseek(txfd, txstart = txpos,
									 SEEK_SET)<0) {
								write_log("seek error on %s", sendf.fname);
								goto giveup;
							}
							txstate = XSENDBLK;
						} else {
							txclose(&txfd, FOP_SKIP);
							qpreset(1);
							flexecute(l);
							l=l->next;
							getfname(&l);
							txstate = XSENDFNAME;
						}
					} else {
						qpreset(1);
						txstate = XDONE;
					}
				}
				if (!(txstate || rxstate)) goto breakout;
				break;

				
				/*-------------------------------------------------------------------*/
				/* ACK to last data block in file                                    */
				/*-------------------------------------------------------------------*/
			case EOFACKPKT:
				if (txstate == XRCVEOFACK || txstate == XRCVFNACK) {
					xmit_retry = 0L;
					if (txstate == XRCVEOFACK) {
						txclose(&txfd, FOP_OK);
						qpfsend();
						flexecute(l);
						l=l->next;
						getfname(&l);
					}
					txstate = XSENDFNAME;
				}
				break;

				/*---------------------------------------------------------------*/
				/* We've got freq         */
				/*---------------------------------------------------------------*/
			case FREQPKT:
				if ((txstate == XRCVFNACK) || (txstate == XDONE)) {
					caps=*(strchr((char *)rxbuf,'\0')+1);
					xmit_retry = 0L;
					write_log("recd janus freq: %s", rxbuf);
					/* TODO FREQS */
					if(cfgs(CFG_EXTRP)||cfgs(CFG_SRIFRP)) {
						slist_t req;
						req.str=(char *)rxbuf;
						req.next=NULL;
						if(freq_ifextrp(&req)) {
							l = fl;
							txstate = XSENDFNAME;
							getfname(&l);
							if(!txfd) txstate = XSENDFREQNAK;
						} else {
							txstate = XSENDFREQNAK;
						}
					} else {
						txstate = XSENDFREQNAK;
					}
				}
				break;
				/*---------------------------------------------------------------*/
				/* Our last file request didn't match anything; move on to next  */
				/*---------------------------------------------------------------*/
			case FREQNAKPKT:
				write_log("janus freq: remote hasn't such file");
				sendpkt (NULL, 0, FRNAKACKPKT);
				break;
				
				/*---------------------------------------------------------------*/
				/* ACK to no matching files for request error; try to end again  */
				/*---------------------------------------------------------------*/
			case FRNAKACKPKT:
				if (txstate == XRCVFRNAKACK) {
					xmit_retry = 0L;
/*					if(l) l=&(*l)->next;	*/
					getfname(&l);
					txstate = XSENDFNAME;
				}
				break;
				
				/*-------------------------------------------------------------------*/
				/* Receiver says "let's try that again."                             */
				/*-------------------------------------------------------------------*/
			case RPOSPKT:
				if (txstate == XSENDBLK || txstate == XRCVEOFACK) {
					if (FETCH32(rxbuf+4) != last_rpostime) {
						last_rpostime = FETCH32(rxbuf+4);
						xmit_retry = 0L;
						if(fseek(txfd,
								 txpos = lasttx = FETCH32(rxbuf),
								 SEEK_SET)<0) {
							write_log("seek error on %s", sendf.fname);
							goto giveup;
						}
						sline("Resending from %ld", txpos);
						if (txblklen >= 128) txblklen >>= 1;
						goodblks = 0;
						goodneeded = goodneeded<<1 | 1;
						txstate = XSENDBLK;
					}
				}
				break;

				/*-------------------------------------------------------------------*/
				/* Debris from end of previous Janus session; ignore it              */
				/*-------------------------------------------------------------------*/
			case HALTACKPKT:
				break;
				
				/*-------------------------------------------------------------------*/
				/* Abort the transfer and quit                                       */
				/*-------------------------------------------------------------------*/
			default:
				write_log("janus: unknown packet type %d",pkttype);
				/* fallthrough */
			case HALTPKT:
			  giveup:
			  if(txfd) {
				  txclose(&txfd, FOP_ERROR);
				  rc=1;
			  }
			  if(rxstate==RRCVBLK) {
				  rxclose(&rxfd, FOP_ERROR);
				  rc=1;
			  }
			  qpreset(0);qpreset(1);
			  goto breakout;
			  
			}  /*  switch (pkttype)  */
		}  /*  while (pkttype)  */
		UHASDATA(200);
	} while (txstate || rxstate);
		
	/*-------------------------------------------------------------------------*/
	/* All done; make sure other end is also finished (one way or another)     */
	/*-------------------------------------------------------------------------*/
  breakout:
	endbatch();
	while(l) {
		if(!l->sendas) flexecute(l);
		l=l->next;
	}
	xfree(txbuf);
	xfree(rxbuf);
	slist_kill(&reqs);
	return rc;
}





/*****************************************************************************/
/* Build and send a packet of any type.                                      */
/* Packet structure is: PKTSTRT,contents,packet_type,PKTEND,crc              */
/* CRC is computed from contents and packet_type only; if PKTSTRT or PKTEND  */
/* get munged we'll never even find the CRC.                                 */
/*****************************************************************************/
void sendpkt(byte *buf, int len, int type)
{
	word crc;

	if((caps&JCAP_CRC32) && type!=FNAMEPKT) {
		sendpkt32(buf,len,type);
		return;
	}

	DEBUG(('J',1,"sendpkt %d bytes, type:%c", len, type));

	BUFCLEAR();
	
	BUFCHAR(DLE);
	BUFCHAR(PKTSTRTCHR^0x40);

	crc=CRC16USD_INIT;
	while(--len>=0) {
		txbyte(*buf);
		crc = CRC16USD_UPDATE(*buf++,crc);
	}

	BUFCHAR(type);
	crc=CRC16USD_FINISH(CRC16USD_UPDATE(type,crc));

	BUFCHAR(DLE);
	BUFCHAR(PKTENDCHR^0x40);

	txbyte(crc>>8);
	txbyte(crc&0xFF);

	BUFFLUSH();
}

void sendpkt32(byte * buf, register int len, int type)
{
	unsigned long crc32;

	BUFCHAR (DLE);
	BUFCHAR (PKTSTRTCHR32 ^ 0x40);

	DEBUG(('J',1,"sendpkt32 %d bytes, type:%c", len, type));

	crc32 = CRC32_INIT;
	while (--len >= 0)
	{
		txbyte(*buf);
		crc32=CRC32_UPDATE(*buf++,crc32);
	}

	BUFCHAR ((byte) type);
	crc32 = CRC32_UPDATE(type,crc32);

	BUFCHAR (DLE);
	BUFCHAR (PKTENDCHR ^ 0x40);

	txbyte ((byte) (crc32 >> 24));
	txbyte ((byte) ((crc32 >> 16) & 0xFF));
	txbyte ((byte) ((crc32 >> 8) & 0xFF));
	txbyte ((byte) (crc32 & 0xFF));

	BUFFLUSH();
}


/*****************************************************************************/
/* Transmit cooked escaped byte(s) corresponding to raw input byte.  Escape  */
/* DLE, XON, and XOFF using DLE prefix byte and ^ 0x40. Also escape          */
/* CR-after-'@' to avoid Telenet/PC-Pursuit problems.                        */
/*****************************************************************************/
void txbyte(int c)
{

	if((txlastc='@' && c==CR)||c==DLE||c==XON||c==XOFF) {
		BUFCHAR(DLE);
		c^=0x40;
	}
	BUFCHAR(c);
	txlastc=c;
}

int rcvbyte(int to)
{
	int c;
	if ((c = GETCHAR(to)) == DLE) {
		if ((c = GETCHAR(timeout)) >= 0) {
			switch (c ^= 0x40) {
			case PKTSTRTCHR:
				c=PKTSTRT;
				break;
			case PKTSTRTCHR32:
				c=PKTSTRT32;
				break;
			case PKTENDCHR:
				c=PKTEND;
				break;
			}
		}
	}
	return c;
}




/*****************************************************************************/
/* Receive, validate, and extract a packet if available.  If a complete      */
/* packet hasn't been received yet, receive and store as much of the next    */
/* packet as possible.  Each call to rcvpkt() will continue accumulating a   */
/* packet until a complete packet has been received or an error is detected. */
/* rxbuf must not be modified between calls to rcvpkt() if NOPKT is returned.*/
/* Returns type of packet received, NOPKT, or BADPKT.  Sets rxblklen.        */
/*****************************************************************************/
byte rcvpkt()
{
	static int is32;
	byte *p;
	int c, i;
	unsigned int pktcrc;
	unsigned int clccrc;

	/*-------------------------------------------------------------------------*/
	/* If not accumulating packet yet, find start of next packet               */
	/*-------------------------------------------------------------------------*/
	if (!(p=rxbufptr)) {
		do c=rcvbyte(0);
		while (c >= 0 || c == PKTEND);

		switch (c) {
		case PKTSTRT:
			p = rxbuf;is32=0;
			break;
		case PKTSTRT32:
			p = rxbuf;is32=1;
			break;
		case ERROR:
		case RCDO:
			return HALTPKT;
		default:
			return NOPKT;
		}
	}

	/*-------------------------------------------------------------------------*/
	/* Accumulate packet data until we empty buffer or find packet delimiter   */
	/*-------------------------------------------------------------------------*/
	while((c=rcvbyte(0))>=0 && p<rxbufmax) *p++=c;

	/*-------------------------------------------------------------------------*/
	/* Handle whichever end-of-packet condition occurred                       */
	/*-------------------------------------------------------------------------*/
	switch (c) {
		/*-----------------------------------------------------------------------*/
		/* PKTEND found; verify valid CRC                                        */
		/*-----------------------------------------------------------------------*/
    case PKTEND:
		pktcrc=0;
		for (i=is32?4:2;i;--i) {
			if ((c=rcvbyte(timeout)) < 0) break;
			pktcrc=(pktcrc<<8)|c;
		}                                                               
		clccrc=(is32?crc32((char *)rxbuf, p-rxbuf):crc16usd((char *)rxbuf, p-rxbuf));
		DEBUG(('J',1,"recvpkt: CRC%d is %08x, got %08x",is32?32:16,clccrc,pktcrc));
		if(!i && pktcrc==clccrc) {
			/*---------------------------------------------------------------*/
			/* Good packet verified; compute packet data length and return   */
			/* packet type                                                   */
			/*---------------------------------------------------------------*/
			rxbufptr = NULL;
			rxblklen = --p - rxbuf;
			return *p;
		}
		/* fallthrough */

    default:
		if (c==RCDO) {
			return HALTPKT;
		} else {
			rxbufptr = NULL;
			sline("Bad packet or CRC error");
			return BADPKT;
		}

		/*-----------------------------------------------------------------------*/
		/* Bad CRC, carrier lost, or buffer overflow from munged PKTEND          */
		/*-----------------------------------------------------------------------*/
    case TIMEOUT:
		rxbufptr = p;
		return NOPKT;

		/*-----------------------------------------------------------------------*/
		/* PKTEND was trashed; discard partial packet and prep for next packet   */
		/*-----------------------------------------------------------------------*/
    case PKTSTRT:
		rxbufptr = rxbuf;
		is32=0;
		sline("Unexpected packet start");
		return BADPKT;
		/*-----------------------------------------------------------------------*/
		/* Emptied buffer; save partial packet and let sender do something       */
		/*-----------------------------------------------------------------------*/
	}
}



/*****************************************************************************/
/* Try REAL HARD to disengage batch session cleanly                          */
/*****************************************************************************/
void endbatch()
{
	int done, timeouts;
	long timeval, brain_dead;

	/*-------------------------------------------------------------------------*/
	/* Tell the other end to halt if it hasn't already                         */
	/*-------------------------------------------------------------------------*/
	done = timeouts = 0;
	brain_dead=t_set(120);
	sendpkt(NULL,0,HALTPKT);
	timeval=t_set(timeout);

	/*-------------------------------------------------------------------------*/
	/* Wait for the other end to acknowledge that it's halting                 */
	/*-------------------------------------------------------------------------*/
	while (!done) {
		if (t_exp(brain_dead)) break;

		switch (rcvpkt()) {
		case NOPKT:
		case BADPKT:
			if (t_exp(timeval)) {
				if (++timeouts > 2) ++done;
				else {
					sendpkt(NULL,0,HALTPKT);
					timeval=t_set(timeout);
				}
			}
			break;

		case HALTPKT:
		case HALTACKPKT:
			++done;
			break;

		default:
			timeouts = 0;
			sendpkt(NULL,0,HALTPKT);
			timeval=t_set(timeout);
			break;
		}
	}

	/*-------------------------------------------------------------------------*/
	/* Announce quite insistently that we're done now                          */
	/*-------------------------------------------------------------------------*/
	for (done=0; done<10; ++done) sendpkt(NULL,0,HALTACKPKT);

}

slist_t *readreq(slist_t *l, char *fname)
{
	FILE *f;
	char s[MAX_PATH], *p;
	
	f=fopen(fname, "rt");
	if(!f) {
		write_log("can't read .req: %s", fname);
		return l;
	}
	
	while(fgets(s,MAX_PATH-1,f)) {
		p=s+strlen(s)-1;
		while(*p=='\r' || *p=='\n') *p--=0;
		slist_add(&l, s);
	}
	fclose(f);

	return l;
}

void getfname(flist_t **l)
{
	while(*l) {
		if(!(*l)->sendas || !(txfd=txopen((*l)->tosend, (*l)->sendas))) {
			flexecute(*l);
		} else {
			break;
		}
		*l=(*l)->next;
	}
	if(txfd) snprintf((char *)txbuf,1024,"%s%c%u %lo %o%c%c",(*l)->sendas,0,sendf.ftot,sendf.mtime,0644,0,OUR_JCAPS);
	else snprintf((char *)txbuf,1024,"%c%c%c",0,0,OUR_JCAPS);
}

void preparereqs(flist_t *l)
{
	while(l) {
		if(l->sendas && l->type==IS_REQ) {
			reqs=readreq(reqs, l->tosend);
			flexecute(l);
		}
		l=l->next;
	}
}
