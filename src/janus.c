/******************************************************************
 * File: janus.c
 * Created at Thu Jan  6 19:24:06 2000 by pk // aaz@ruxy.org.ru
 * Janus protocol implementation
 * $Id: janus.c,v 1.1.1.1 2000/07/18 12:37:20 lev Exp $
 ******************************************************************/
/*---------------------------------------------------------------------------*/
/*                    Opus Janus revision 0.22,  1- 9-88                     */
/*                                                                           */
/*                The Opus Computer-Based Conversation System                */
/*           (c) Copyright 1987, Rick Huebner, All Rights Reserved           */
/*---------------------------------------------------------------------------*/
#include <sys/stat.h>
#include <sys/time.h>
#include "mailer.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "janus.h"
#include "qipc.h"
#include "globals.h"
#include "ver.h"
#include "defs.h"

/*****************************************************************************/
/* Super-duper neato-whizbang full-duplex streaming ACKless batch file       */
/* transfer protocol for use in Opus-to-Opus mail sessions                   */
/*****************************************************************************/
void janus()
{
	byte   pkttype;          /* Type of packet last received                   */
	word   blklen;           /* Length of last data block sent                 */
	word   goodneeded;       /* # good blocks to send before upping txblklen   */
	word   goodblks;         /* Number of good blocks sent at this block size  */
	word   rpos_count;       /* Number of RPOS packets sent at this position   */
	long   xmit_retry;       /* Time to retransmit lost FNAMEPKT or EOF packet */
	long   lasttx;           /* Position within file of last data block we sent*/
	long   txsttime;         /* Time at which we started sending current file  */
	long   rxsttime;         /* Time at which we started receiving current file*/
	long   txoldpos;         /* Last transmission file position displayed      */
	long   rxoldpos;         /* Last reception file position displayed         */
	long   rpos_retry;       /* Time at which to retry RPOS packet             */
	long   brain_dead;       /* Time at which to give up on other computer     */
	long   rpos_sttime;      /* Time at which we started current RPOS sequence */
	long   last_rpostime;    /* Timetag of last RPOS which we performed        */
	long   last_blkpos;      /* File position of last out-of-sequence BLKPKT   */


	rxbufmax = rxbuf+BUFMAX+8;

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

	getfname();
  
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
			log("other end died");  /* "He's dead, Jim." */
			goto giveup;
		}

		/*-----------------------------------------------------------------------*/
		/* If we're tired of waiting for an ACK, try again                       */
		/*-----------------------------------------------------------------------*/
		if (xmit_retry) {
			if (t_exp(xmit_retry)) {
				sline("timeout waiting for ACK - retry");
				xmit_retry = 0L;

				switch (txstate) {
				case XRCVFNACK:
					txstate = XSENDFNAME;
					break;
				case XRCVEOFACK:
					if(seek(txfd, txpos=lasttx, SEEK_SET)) {
						log("seek error");
						goto giveup;
					}
					txstate = XSENDBLK;
					break;
				}
			}
		}

		/*-----------------------------------------------------------------------*/
		/* Transmit next part of file, if any                                    */
		/*-----------------------------------------------------------------------*/
		switch (txstate) {
		case XSENDBLK:
			*((long *)txbuf) = lasttx = txpos;
			blklen = fread(txbuf+sizeof(txpos), txblklen, txfd);
			if(blklen<1) {
				log("read error");
				goto giveup;
			}
			txpos += blklen;
			sendpkt(txbuf, sizeof(txpos)+blklen, BLKPKT);

			// UPDATE SEND STATUS 
			
			if (txpos >= sendf.ftot || blklen < txblklen) {
				long_set_timer(&xmit_retry,timeout);
				txstate  = XRCVEOFACK;
			} else long_set_timer(&brain_dead,120);

			if (txblklen < txmaxblklen && ++goodblks > goodneeded) {
				txblklen <<= 1;
				goodblks = 0;
			}
			break;

		case XSENDFNAME:
			blklen = strchr( strchr(txbuf,'\0')+1, '\0') - txbuf + 1;
			sendpkt(txbuf,blklen,FNAMEPKT);
			txoldpos = txoldlen = txoldeta = -1;
			xmit_retry=t_set(timeout);
			txstate = XRCVFNACK;
			break;
		}

		/*-----------------------------------------------------------------------*/
		/* Catch up on our reading; receive and handle all outstanding packets   */
		/*-----------------------------------------------------------------------*/
		while (pkttype = rcvpkt()) {
			switch (pkttype) {

				/*-------------------------------------------------------------------*/
				/* File data block or munged block                                   */
				/*-------------------------------------------------------------------*/
			case BADPKT:
			case BLKPKT:
				if (rxstate == RRCVBLK) {
					if (pkttype == BADPKT || *((long *)rxbuf) != rxpos) {
						if (pkttype == BLKPKT) {
							if (*((long *)rxbuf) < last_blkpos) rpos_count = 0;
							last_blkpos = *((long *)rxbuf);
						}
						if (t_exp(rpos_retry))  {
							if (rpos_count > 5) rpos_count = 0;
							if (++rpos_count == 1) time(&rpos_sttime);
							log("Bad packet at %ld", rxpos);
							*((long *)rxbuf) = rxpos;
							*((long *)(rxbuf+sizeof(rxpos))) = rpos_sttime;
							sendpkt(rxbuf, sizeof(rxpos)+sizeof(rpos_sttime), RPOSPKT);
							rpos_retry=t_set(timeout/2);
						}
					} else {
						brain_dead=t_set(120);
						last_blkpos = rxpos;
						rpos_retry = rpos_count = 0;
						if(fwrite(rxbuf+sizeof(rxpos),
								  rxblklen -= sizeof(rxpos), rxfd)<0) {
							log("write error");goto giveup;
						}
						rxpos += rxblklen;

						// UPDATE STATUS RECEIVE

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
				sendpkt((byte *)&rxpos,sizeof(rxpos),FNACKPKT);
				rxoldpos = rxoldlen = -1;
				if(rstate==RRCVFNAME) {
					if(!rxbuf[0]) rxstate=RDONE;
					else {
						scanf(rxbuf+strlen(rxbuf)+1, "%lu %lo %*o",
							  &recvf.ftot, &recvf.mtime);
						switch(rxopen(rxbuf, recvf.ftot, recvf.mtime, &rxfd)){
						case FOP_SUSPEND:
							goto breakout;
						case FOP_OK:
							rxstate=RRCVBLK;
						case FOP_SKIP:
							rxpos=rxstart=recvf.foff;
						}
					}
				}
				if (!(txstate || rxstate)) goto breakout;
				break;

				/*-------------------------------------------------------------------*/
				/* ACK to filename packet we just sent                               */
				/*-------------------------------------------------------------------*/
			case FNACKPKT:
				if (txstate == XRCVFNACK) {
					xmit_retry = 0L;
					if(txfd) {
						if ((txpos = *((long *)rxbuf)) > -1L) {
							sendf.soff=txpos;
							if(fseek(txfd, txstart = txpos,
									 SEEK_SET)<0) {
								log("seek error");
								goto giveup;
							}
							txstate = XSENDBLK;
						} else {
							txclose(txfd, FOP_SKIP);
							getfname(GOOD_XFER);
							txstate = XSENDFNAME;
						}
					} else txstate = XDONE;
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
						getfname(GOOD_XFER);
					}
					txstate = XSENDFNAME;
				}
				break;

				/*-------------------------------------------------------------------*/
				/* Receiver says "let's try that again."                             */
				/*-------------------------------------------------------------------*/
			case RPOSPKT:
				if (txstate == XSENDBLK || txstate == XRCVEOFACK) {
					if (*((long *)(rxbuf+sizeof(txpos))) != last_rpostime) {
						last_rpostime = *((long *)(rxbuf+sizeof(txpos)));
						xmit_retry = 0L;
						if(fseek(Txfile,
								 txpos = lasttx = *((long*)rxbuf),
								 SEEK_SET)<0) {
							log("seek error");
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
				log("Unknown packet type %d",pkttype);
				/* fallthrough */
			case HALTPKT:
			  giveup:
			  log("Session aborted");
			  if(txfd) txclose(&txfd, FOP_ERROR);
			  if(rxstate==RRCVBLK) rxclose(&txfd, FOP_ERROR);
			  goto breakout;
			  
			}  /*  switch (pkttype)  */
		}  /*  while (pkttype)  */
	} while (txstate || rxstate);
		
	/*-------------------------------------------------------------------------*/
	/* All done; make sure other end is also finished (one way or another)     */
	/*-------------------------------------------------------------------------*/
  breakout:
	endbatch();
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

	PUTCHAR(DLE);
	PUTCHAR(PKTSTRTCHR^0x40);

	crc=0;
	while(--len>=0) txbyte(*buf);

	PUTCHAR(type);
	crc=crc16(crc,type);

	PUTCHAR(DLE);
	PUTCHAR(PKTENDCHR^0x40);

	txbyte(crc>>8);
	txbyte(crc&0xFF);
}


/*****************************************************************************/
/* Transmit cooked escaped byte(s) corresponding to raw input byte.  Escape  */
/* DLE, XON, and XOFF using DLE prefix byte and ^ 0x40. Also escape          */
/* CR-after-'@' to avoid Telenet/PC-Pursuit problems.                        */
/*****************************************************************************/
void txbyte(int c)
{

	if((txlastc='@' && c==CR)||c==DLE||c==XON||c==XOFF) {
		PUTCHAR(DLE);
		c^=0x40;
	}
	PUTCHAR(c);
	txlastc=c;
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
	static word crc;
	byte *p;
	int c;
	word pktcrc;

	/*-------------------------------------------------------------------------*/
	/* Abort transfer if operator pressed ESC                                  */
	/*-------------------------------------------------------------------------*/
/*  	if (KBD_GiveUp()) { */
/*  		j_status(GENERIC_ERROR,KBD_msg); */
/*  		return HALTPKT; */
/*  	} */

	/*-------------------------------------------------------------------------*/
	/* If not accumulating packet yet, find start of next packet               */
	/*-------------------------------------------------------------------------*/
	if (!(p=rxbufptr)) {
		do c=GETCHAR(0);
		while (c >= 0 || c == PKTEND);

		switch (c) {
		case PKTSTRT:
			p = rxbuf;
			crc = 0;
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
	while((c=GETCHAR(0))>=0 && p<rxbufmax) *p++=c;

	/*-------------------------------------------------------------------------*/
	/* Handle whichever end-of-packet condition occurred                       */
	/*-------------------------------------------------------------------------*/
	switch (c) {
		/*-----------------------------------------------------------------------*/
		/* PKTEND found; verify valid CRC                                        */
		/*-----------------------------------------------------------------------*/
    case PKTEND:
		if(GETCHAR(timeout)>=0) {
			pktcrc=c<<8;
			if (GETCHAR(timeout)>= 0) {
				if((pktcrc|c)==crc16(rxbuf, p-rxbuf)) {
					/*---------------------------------------------------------------*/
					/* Good packet verified; compute packet data length and return   */
					/* packet type                                                   */
					/*---------------------------------------------------------------*/
					rxbufptr = NULL;
					rxblklen = --p - rxbuf;
					return *p;
				}
			}
		}
		/* fallthrough */

		/*-----------------------------------------------------------------------*/
		/* Bad CRC, carrier lost, or buffer overflow from munged PKTEND          */
		/*-----------------------------------------------------------------------*/
    default:
		if (c==CARRIER) {
			return HALTPKT;
		} else {
			rxbufptr = NULL;
			return BADPKT;
		}

		/*-----------------------------------------------------------------------*/
		/* Emptied buffer; save partial packet and let sender do something       */
		/*-----------------------------------------------------------------------*/
    case BUFEMPTY:
		rxbufptr = p;
		return NOPKT;

		/*-----------------------------------------------------------------------*/
		/* PKTEND was trashed; discard partial packet and prep for next packet   */
		/*-----------------------------------------------------------------------*/
    case PKTSTRT:
		rxbufptr = rxbuf;
		crc = 0;
		return BADPKT;
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
	long_set_timer(&brain_dead,120);
	goto reject;

	/*-------------------------------------------------------------------------*/
	/* Wait for the other end to acknowledge that it's halting                 */
	/*-------------------------------------------------------------------------*/
	while (!done) {
		if (long_time_gone(&brain_dead)) break;

		switch (rcvpkt()) {
		case NOPKT:
		case BADPKT:
			if (long_time_gone(&timeval)) {
				if (++timeouts > 2) ++done;
				else goto reject;
			}
			break;

		case HALTPKT:
		case HALTACKPKT:
			++done;
			break;

		default:
			timeouts = 0;
		  reject:
			sendpkt(NULL,0,HALTPKT);
			long_set_timer(&timeval,timeout);
			break;
		}
	}

	/*-------------------------------------------------------------------------*/
	/* Announce quite insistently that we're done now                          */
	/*-------------------------------------------------------------------------*/
	for (done=0; done<10; ++done) sendpkt(NULL,0,HALTACKPKT);

}

