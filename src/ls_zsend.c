/**********************************************************
 * File: ls_zsend.c
 * Created at Sun Oct 29 18:51:46 2000 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: ls_zsend.c,v 1.3 2000/11/10 12:37:21 lev Exp $
 **********************************************************/
/*

   ZModem file transfer protocol. Written from scratches.
   Support CRC16, CRC32, RLE Encoding, variable header, ZedZap (big blocks).
   Send files.

*/
#include "mailer.h"
#include "ftn.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "qconf.h"
#include "ver.h"
#include "qipc.h"
#include "globals.h"
#include "defs.h"
#include "ls_zmodem.h"


/* Init sender, preapre to send files */
int ls_zsendinit(int Protocol, int effbaud)
{
	int tries = 0;	
	int hlen = 0;
	int rc;
	int rxOptions;

	ls_Protocol=Protocol;

 	PUTSTR("rz\r");
	ls_storelong(ls_txHdr,0L);
	if((rc=ls_zsendhhdr(ZRQINIT,4,ls_txHdr))<0) return rc;
	do {
		switch((rc=ls_zrecvhdr(ls_rxHdr,&hlen,5))) {
		case ZINIT:			/* Ok, We got INIT! */
			rxOptions=STOH(((ls_rxHdr[LSZ_F1]&0xFF)<<8) | (ls_rxHdr[LSZ_F0]&0xFF));
			ls_rxCould = (rxOptions&(LSZ_RXCANDUPLEX|LSZ_RXCANOVIO|LSZ_RXCANBRK));
			/* Strip RLE from ls_Protocol, if peer could not use RLE */
			if(!(rxOptions&LSZ_RXCANRLE)) ls_Protocol&=(~LSZ_OPTRLE);
			/* Strip CRC32 from ls_Protocol, if peer could not use CRC32 */
			if(!(rxOptions&LSZ_RXCANFC32)) ls_Protocol&=(~LSZ_OPTCRC32);
			/* Set EscapeAll if peer want it */
			if(rxOptions&LSZ_RXWNTESCCTL) ls_Protocol|=LSZ_OPTESCALL;
			/* Strip VHeaders from ls_Protocol, if peer could not use VHDR */
			if(!(rxOptions&LSZ_RXCANVHDR)) ls_Protocol&=(~LSZ_OPTVHDR);
			/* Ok, options are ready */
			ls_txWinSize=STOH(((ls_rxHdr[LSZ_P1]&0xFF)<<8) | (ls_rxHdr[LSZ_P0]&0xFF));

			/* Ok, now we could calculate real max frame size and initial block size */
			ls_txMaxBlockSize = ls_Protocol&LSZ_OPTZEDZAP?8192:1024;
			if(ls_txWinSize && ls_txMaxBlockSize>ls_txWinSize) {
				ls_txMaxBlockSize=1;
				for(ls_txMaxBlockSize=1;ls_txMaxBlockSize<ls_txWinSize;ls_txMaxBlockSize<<=1;);
				ls_txMaxBlockSize>>=1;
				if(ls_txMaxBlockSize<32) ls_txWinSize=ls_txMaxBlockSize=32;
			}

            if(effbaud<2400) ls_txCurBlockSize = 256;
			else if(effbaud>=2400 && effbaud<4800) ls_txCurBlockSize = 512;
			else ls_txCurBlockSize = 1024;
            
            if(ls_Protocol&LSZ_OPTZEDZAP) {
	            if(effbaud>=7200 && effbaud<9600) ls_txCurBlockSize = 2048;
				else if(effbaud>=9600 && effbaud<14400) ls_txCurBlockSize = 4096;
				else if(effbaud>=14400) ls_txCurBlockSize = 8192;
			}
			if(ls_txCurBlockSize<ls_txMaxBlockSize) ls_txCurBlockSize=ls_txMaxBlockSize;

			/* Allocate memory for send buffer */
			ls_BufferSize=(ls_txMaxBlockSize+16)*2;	/* Maybe, we will need to escape ALL CHARS */
			if(NULL==(rxbuf=malloc(ls_BufferSize))) return LSZ_ERROR;
			if(NULL==(txbuf=malloc(ls_BufferSize))) return LSZ_ERROR;

			/* We don't need Attn or escaping -- don't send ZSINIT */
			return LSZ_OK;
		case ZCHALLENGE:	/* Return number to peer, he is paranoid */
			ls_storelong(ls_txHdr,0L);
			if((rc=ls_zsendhhdr(ZACK,4,ls_txHdr))<0) return rc;
			break;
		case LSZ_TIMEOUT:	/* Send ZRQINIT again */
			ls_storelong(ls_txHdr,0L);
			if((rc=ls_zsendhhdr(ZRQINIT,4,ls_txHdr))<0) return rc;
			break;
		case LSZ_RCDO:
		case LSZ_ERROR:		/* Return this result */
		case LSZ_CAN:
			return rc;
		case ZRQINIT:		/* He want our Init? */
			if(ZCOMMAND==ls_rxHdr[ZF0]) break;
		case ZCOMMAND:		/* We don't support it! */
		case LSZ_BADCRC:	/* Send NAK */
		default:
			ls_zsendhhdr(ZNAK,0,ls_txHdr);
			tries++;
		}
	} while(tries < 10);
	return LSZ_TIMEOUT;
}



/* Send one file to peer */
