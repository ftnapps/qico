/**********************************************************
 * File: ls_zsend.c
 * Created at Sun Oct 29 18:51:46 2000 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: ls_zsend.c,v 1.11 2001/01/24 11:46:01 lev Exp $
 **********************************************************/
/*

   ZModem file transfer protocol. Written from scratches.
   Support CRC16, CRC32, variable header, ZedZap (big blocks) and DirZap.
   Global variables, common functions.

*/
#include "headers.h"
#include "defs.h"
#include "ls_zmodem.h"
#include "qipc.h"

/* Finish sending after CANs, ZABORTs and ZFINs in bad places */
int ls_finishsend()
{
	ls_storelong(ls_txHdr,0);
	ls_zsendhhdr(ZFIN,4,ls_txHdr);
	return 0;
}

/* Send ZSINIT and wait for ZACK, skip ZRINIT, ZCOMMAND, answer on ZCHALLENGE */
int ls_zsendsinit(char *attstr)
{
	int trys = 0;
	int retransmit = 1;
	int rc;
	int hlen;
	int l;

#ifdef Z_DEBUG
	write_log("ls_zsendsinit: '%s'",attstr?attstr:"(null)");
#endif

	if (attstr)  {
		if (strlen(attstr) > LSZ_MAXATTNLEN-1) attstr[LSZ_MAXATTNLEN-1] = '\x00';
		strcpy(txbuf,attstr);
	} else {
		txbuf[0] = '\x00';
	}
	l = strlen(txbuf) + 1;

	do {
		if(retransmit) {
#ifdef Z_DEBUG2
			write_log("ls_zsendsinit: resend ZSINIT");
#endif
			/* We don't support ESC8, so don't ask for it in any case */
			ls_txHdr[LSZ_F0] = (ls_Protocol&LSZ_OPTESCAPEALL)?LSZ_TXWNTESCCTL:0;
			ls_txHdr[LSZ_F1] = ls_txHdr[LSZ_F2] = ls_txHdr[LSZ_F3] = 0;
			if((rc=ls_zsendbhdr(ZSINIT,4,ls_txHdr))<0) return rc;
			if((rc=ls_zsenddata(txbuf,l,ZCRCW))) return rc;
			retransmit = 0;
			trys++;
		}
		switch((rc=ls_zrecvhdr(ls_rxHdr,&hlen,ls_HeaderTimeout))) {
		case ZRINIT:		/* Skip it */
#ifdef Z_DEBUG2
			write_log("ls_zsendsinit: ZRINIT");
#endif
			break;
		case ZACK:			/* Ok */
#ifdef Z_DEBUG2
			write_log("ls_zsendsinit: ZACK");
#endif
			return LSZ_OK;
		case ZCHALLENGE:	/* Return number to peer, he is paranoid */
#ifdef Z_DEBUG2
			write_log("ls_zsendsinit: CHALLENGE");
#endif
			ls_storelong(ls_txHdr,ls_fetchlong(ls_rxHdr));
			if((rc=ls_zsendhhdr(ZACK,4,ls_txHdr))<0) return rc;
			break;
		case LSZ_BADCRC:
		case ZCOMMAND:
#ifdef Z_DEBUG2
			write_log("ls_zsendsinit: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			ls_storelong(ls_txHdr,0);
			if((rc=ls_zsendhhdr(ZNAK,4,ls_txHdr))<0) return rc;
			break;
		case ZNAK:			/* Retransmit */
		case LSZ_TIMEOUT:
#ifdef Z_DEBUG2
			write_log("ls_zsendsinit: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			retransmit = 1;
			break;
		default:
#ifdef Z_DEBUG
			write_log("ls_zsendsinit: something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			if(rc<0) return rc;
			break;
		}
	} while(trys < 10);
#ifdef Z_DEBUG
	write_log("ls_zsendsinit: timeout or something other: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
	return rc;
}

/* Init sender, preapre to send files (initialize timeouts too!) */
int ls_zinitsender(int protocol, int baud, int window, char *attstr)
{
	int trys = 0;
	int retransmit = 1;
	int hlen = 0;
	int rc;
	int rxOptions;	/* Options from ZRINIT header */
	int zfins = 0;	/* ZFIN counter -- we will trust only MAY of them on this stage */

#ifdef Z_DEBUG
	write_log("ls_zinitsender: %08x, %d baud, %d bytes",protocol,baud,window);
#endif

	/* Set all options to requested state -- this may be alerted by other side */
	ls_Protocol = protocol;

	/* Maximum block size -- by protocol, may be reduced by window size later */
	ls_MaxBlockSize = ls_Protocol&LSZ_OPTZEDZAP?8192:1024;

	/* Calculate timeouts */
	/* Timeout for header waiting, if no data sent -- 3*TransferTime or 5 seconds */
	ls_HeaderTimeout = (LSZ_MAXHLEN * 30) / baud;
	ls_HeaderTimeout = ls_HeaderTimeout>5?ls_HeaderTimeout:5;
	/* Timeout for data packet (3*TransferTime or 30 seconds) */
	ls_DataTimeout = (ls_MaxBlockSize * 30) / baud;
	ls_DataTimeout = ls_DataTimeout>30?ls_DataTimeout:30;

	ls_SkipGuard = (ls_Protocol&LSZ_OPTSKIPGUARD)?1:0;
	ls_SerialNum = 1;
    
	/* Why we need to send this? Old, good times... */
	PUTSTR("rz\r");
	do {
		if(retransmit) {
			/* Send first ZRQINIT (do we need it?) */
#ifdef Z_DEBUG2
			write_log("ls_zinitsender: resend ZRQINIT");
#endif
			ls_storelong(ls_txHdr,0L);
			if((rc=ls_zsendhhdr(ZRQINIT,4,ls_txHdr))<0) return rc;
			retransmit = 0;
			trys++;
		}
		switch((rc=ls_zrecvhdr(ls_rxHdr,&hlen,ls_HeaderTimeout))) {
		case ZRINIT:			/* Ok, We got RINIT! */
			rxOptions=STOH(((ls_rxHdr[LSZ_F1]&0xff)<<8) | (ls_rxHdr[LSZ_F0]&0xff));
			/* What receiver could (in hardware) -- Duplex, sim. I/O, send break signals */
			ls_rxCould = (rxOptions&(LSZ_RXCANDUPLEX|LSZ_RXCANOVIO|LSZ_RXCANBRK));
			/* Strip RLE from ls_Protocol, if peer could not use RLE (WE COULD NOT RLE in ANY CASE!) */
			if(!(rxOptions&LSZ_RXCANRLE)) ls_Protocol&=(~LSZ_OPTRLE);
			/* Strip CRC32 from ls_Protocol, if peer could not use CRC32 */
			if(!(rxOptions&LSZ_RXCANFC32)) ls_Protocol&=(~LSZ_OPTCRC32);
			/* Set EscapeAll if peer want it */
			if(rxOptions&LSZ_RXWNTESCCTL) ls_Protocol|=LSZ_OPTESCAPEALL;
			/* Strip VHeaders from ls_Protocol, if peer could not use VHDR */
			if(!(rxOptions&LSZ_RXCANVHDR)) ls_Protocol&=(~LSZ_OPTVHDR);
			/* Ok, options are ready */
			/* Fetch window size */
			ls_txWinSize=STOH(((ls_rxHdr[LSZ_P1]&0xFF)<<8) | (ls_rxHdr[LSZ_P0]&0xFF));
			/* Override empty or big window by our window (if not emty too) */
			if(window && (!ls_txWinSize || ls_txWinSize > window)) ls_txWinSize = window;

#ifdef Z_DEBUG2
			write_log("ls_zinitsender: RINIT OK, effproto: %08x, winsize: %d",ls_Protocol,ls_txWinSize);
#endif

			/* Ok, now we could calculate real max frame size and initial block size */
			if(ls_txWinSize && ls_MaxBlockSize>ls_txWinSize) {
				for(ls_MaxBlockSize=1;ls_MaxBlockSize<ls_txWinSize;ls_MaxBlockSize<<=1);
				ls_MaxBlockSize >>= 1;
				if(ls_MaxBlockSize<32) ls_txWinSize=ls_MaxBlockSize=32;
			}

                        if(baud<2400) ls_txCurBlockSize = 256;
			else if(baud>=2400 && baud<4800) ls_txCurBlockSize = 512;
			else ls_txCurBlockSize = 1024;
            
                        if(ls_Protocol&LSZ_OPTZEDZAP) {
                                if(baud>=7200 && baud<9600) ls_txCurBlockSize = 2048;
				else if(baud>=9600 && baud<14400) ls_txCurBlockSize = 4096;
				else if(baud>=14400) ls_txCurBlockSize = 8192;
			}
			if(ls_txCurBlockSize<ls_MaxBlockSize) ls_txCurBlockSize=ls_MaxBlockSize;
#ifdef Z_DEBUG2
			write_log("ls_zinitsender: Block sizes: %d, %d",ls_MaxBlockSize,ls_txCurBlockSize);
#endif

			/* Allocate memory for send buffer */
			if(NULL==(txbuf=malloc((ls_MaxBlockSize+16)))) return LSZ_ERROR;

			/* Send ZSINIT, if we need it */
			if(attstr || (!(rxOptions&LSZ_RXWNTESCCTL) && (ls_Protocol&LSZ_OPTESCAPEALL)))
				return ls_zsendsinit(attstr);
			else
				return LSZ_OK;
		case ZCHALLENGE:	/* Return number to peer, he is paranoid */
#ifdef Z_DEBUG2
			write_log("ls_zinitsender: CHALLENGE");
#endif
			ls_storelong(ls_txHdr,ls_fetchlong(ls_rxHdr));
			if((rc=ls_zsendhhdr(ZACK,4,ls_txHdr))<0) return rc;
			break;
		case ZNAK:			/* Send ZRQINIT again */
		case LSZ_TIMEOUT:
#ifdef Z_DEBUG2
			write_log("ls_zinitsender: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			retransmit = 1;
			break;
		case ZFIN:			/* ZFIN from previous session? Or may be real one? */
#ifdef Z_DEBUG2
			write_log("ls_zinitsender: ZFIN %d",zfins);
#endif
			if(++zfins == LSZ_TRUSTZFINS) return LSZ_ERROR;
			break;
		case LSZ_BADCRC:	/* Please, resend */
		case ZCOMMAND:		/* We don't support it! */
#ifdef Z_DEBUG2
			write_log("ls_zinitsender: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
		case ZABORT:		/* Abort this session -- we trust in ABORT! */
#ifdef Z_DEBUG2
			write_log("ls_zinitsender: ABORT");
#endif
			return LSZ_ERROR;
		default:
#ifdef Z_DEBUG
			write_log("ls_zinitsender: something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			if(rc<0) return rc;
			break;
		}
	} while(trys < 10);
#ifdef Z_DEBUG
	write_log("ls_zinitsender: timeout or something other: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
	return rc;
}

/* Send file information to peer, get start position from them.
   Return packet type -- ZRPOS, ZSKIP, ZFERR, ZABORT or ZFIN (may be any error, too) */
int ls_zsendfinfo(ZFILEINFO *f, unsigned long sernum, long *pos)
{
	char finfolen;
	int trys = 0;
	int retransmit = 1;
	int rc;
	int hlen;
	char *p;
	int crc = LSZ_INIT_CRC32;
	int c;
	int cnt;
	int len;
	long sn;

#ifdef Z_DEBUG
	write_log("ls_zsendfinfo: %s, %d, %d, %d, %d, %d",f->name,f->size,f->mtime,sernum,f->filesleft,f->bytesleft);
#endif

	txbuf[0] = '\x00';
	strcpy(txbuf,f->name);
	p = txbuf + strlen(f->name);
	*p = '\x00'; p++;
	sprintf(p,"%ld %lo %o %o %ld %ld",f->size,f->mtime,(int)0,(int)sernum,f->filesleft,f->bytesleft);
	finfolen = strlen(txbuf) + strlen(p) + 2;

	do {
		if(retransmit) {
#ifdef Z_DEBUG2
			write_log("ls_zsendfinfo: resend ZFILE");
#endif
			ls_txHdr[LSZ_F0] = LSZ_CONVBIN | LSZ_CONVRECOV;
			ls_txHdr[LSZ_F1] = 0; /* No managment */
			ls_txHdr[LSZ_F2] = 0; /* No compression/encryption */
			ls_txHdr[LSZ_F3] = 0; /* No sparse files or variable headers */
			if((rc=ls_zsendbhdr(ZFILE,4,ls_txHdr))<0) return rc;
			if((rc=ls_zsenddata(txbuf,finfolen,ZCRCW))<0) return rc;
			retransmit = 0;
			trys++;
		}
		switch((rc=ls_zrecvhdr(ls_rxHdr,&hlen,ls_HeaderTimeout))) {
		case ZRPOS:				/* Ok, he want our file */
#ifdef Z_DEBUG2
			write_log("ls_zsendfinfo: ZRPOS at %d",ls_fetchlong(ls_rxHdr));
#endif
			*pos = ls_fetchlong(ls_rxHdr);
			return ZRPOS;
		case ZSKIP:		/* Skip */
		case ZFERR:		/* Refuse */
			/* Check for double-skip protection */
			sn = ls_fetchlong(ls_rxHdr);
			if(ls_SkipGuard && sn && sn == sernum - 1) {	/* Here is skip protection */
#ifdef Z_DEBUG
				write_log("ls_zsendfinfo: double-skip protection! for %d, %s (this file: %d, got: %d, ls_SN: %d)",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET],sernum,sn,ls_SerialNum);
#endif
				ls_storelong(ls_txHdr,0);
				if((rc=ls_zsendhhdr(ZNAK,4,ls_txHdr))<0) return rc;
				break;								/* We don't need to skip this file */
			} else if(sn != sernum) {
#ifdef Z_DEBUG
				write_log("ls_zsendfinfo: turn off double-skip protection in %d, %s (%d)",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET],sn);
#endif
				ls_SkipGuard = 0;
			}
			/* Fall through */
		case ZABORT:	/* Abort this session */
		case ZFIN:		/* Finish this session */
#ifdef Z_DEBUG2
			write_log("ls_zsendfinfo: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			return rc;
		case ZCRC:		/* Send CRC to peer */
#ifdef Z_DEBUG2
			write_log("ls_zsendfinfo: ZCRC for %d bytes",ls_fetchlong(ls_rxHdr));
#endif
			len = ls_fetchlong(ls_rxHdr);
			if(!len) len = f->size;
			cnt = 0;
			fseek(txfd,0,SEEK_SET);
			while((cnt++ < len) && ((c = getc(txfd)) > 0)) { crc = LSZ_UPDATE_CRC32(c,crc); }
			clearerr(txfd);
			fseek(txfd,0,SEEK_SET);
			crc = LSZ_FINISH_CRC32(crc);
			ls_storelong(ls_txHdr,crc);
			if((rc=ls_zsendhhdr(ZCRC,4,ls_txHdr))<0) return rc;
			break;
		case ZNAK:
		case LSZ_TIMEOUT:
#ifdef Z_DEBUG2
			write_log("ls_zsendfinfo: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			retransmit = 1;
			break;
		case LSZ_BADCRC:
#ifdef Z_DEBUG2
			write_log("ls_zsendfinfo: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			ls_storelong(ls_txHdr,0);
			if((rc=ls_zsendhhdr(ZNAK,4,ls_txHdr))<0) return rc;
			break;
		default:
#ifdef Z_DEBUG
			write_log("ls_zsendfinfo: something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			if(rc<0) return rc;
			break;
		}
	} while (trys < 10);
#ifdef Z_DEBUG
	write_log("ls_zsendfinfo: timeout or something other: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
	return rc;
}

/* Intrnal function to process ZRPOS */
int ls_zrpos(int newpos)
{
#ifdef Z_DEBUG
	write_log("ls_zrpos: ZRPOS to %d",newpos);
#endif
	txpos = newpos;

	if(txpos == ls_txLastRepos) {
		if(++ls_txReposCount > 10) {
#ifdef Z_DEBUG
			write_log("ls_zrpos: ZRPOS to %d limit reached",txpos);
#endif
			return LSZ_ERROR;
		}
	} else {
		ls_txReposCount = 0;
		ls_txLastRepos = txpos;
	}
	ls_txLastACK = txpos;	/* Drop window */
	clearerr(txfd);			/* May be EOF */
	if(fseek(txfd,txpos,SEEK_SET)) {
#ifdef Z_DEBUG
		write_log("ls_zrpos: ZRPOS to %d seek error",txpos);
#endif
		return LSZ_ERROR;
	}
	if(ls_txCurBlockSize > 32) ls_txCurBlockSize >>= 1;
	ls_txGoodBlocks = 0;
	return LSZ_OK;
}

/* Send one file to peer */
int ls_zsendfile(ZFILEINFO *f, unsigned long sernum)
{

	enum sfMODE {
		sfStream,			/* No window requested (use ZCRCG) */
		sfSlidingWindow,	/* Window requested, could do fullduplex (use stream with ZCRCQ and sliding window) */
		sfBuffered			/* Window requested, couldn't use fullduplex (use frames ZCRCG + ZCRCW) */
	} mode;

	int trys = 0;
	int rc;
	int bread = 0;
	int frame;
	int hlen;
	int needack = 0;

#ifdef Z_DEBUG
	write_log("ls_zsendfile: %s",f->name);
#endif

	switch((rc = ls_zsendfinfo(f,sernum,&txpos))) {
	case ZRPOS:		/* Ok, It's OK! */
#ifdef Z_DEBUG2
		write_log("ls_zsendfile: ZRPOS to %d",txpos);
#endif
		break;
	case ZSKIP:		/* Skip it */
	case ZFERR:		/* Suspend it */
#ifdef Z_DEBUG2
		write_log("ls_zsendfile: %s",LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
		return rc;
	case ZABORT:
	case ZFIN:		/* Session is aborted */
#ifdef Z_DEBUG2
		write_log("ls_zsendfile: %s",LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
		ls_finishsend();
		return LSZ_ERROR;
	default:
		if(rc < 0) return rc;
#ifdef Z_DEBUG
		write_log("ls_zsendfile: Strange anwfer on ZFILE:  %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
		return LSZ_ERROR;
	}

	if(fseek(txfd,txpos,SEEK_SET)) {
#ifdef Z_DEBUG
		write_log("ls_zsendfile: Seek to %d error",txpos);
#endif
		return LSZ_ERROR;
	}

	/* Send file data */
	if(ls_txWinSize) {
		if(ls_rxCould & LSZ_RXCANDUPLEX) mode = sfSlidingWindow;
		else mode = sfBuffered;
	} else {
		mode = sfStream;
	}
	frame = ZCRCW;
#ifdef Z_DEBUG
	write_log("ls_zsendfile: mode %s",sfStream==mode?"stream":(sfBuffered==mode?"buffered":"sliding window"));
#endif

	while(!feof(txfd)) {
		/* We need to send ZDATA if previous frame was ZCRCW
		Also, frame will be ZCRCW, if it is after RPOS */
		if(ZCRCW == frame) { 
#ifdef Z_DEBUG
			write_log("ls_zsendfile: send ZDATA at %d",txpos);
#endif
			ls_storelong(ls_txHdr,txpos);
			if((rc=ls_zsendbhdr(ZDATA,4,ls_txHdr))<0) return rc;
		}
		/* Send frame of data */
		bread = fread(txbuf,1,ls_txCurBlockSize,txfd);
		if(bread < 0) {
#ifdef Z_DEBUG
			write_log("ls_zsendfile: read error at %d",txpos);
#endif
			return LSZ_ERROR;
		}
		/* Select sub-frame type */
		if(bread < ls_txCurBlockSize) {		/* This is last sub-frame -- EOF */
#ifdef Z_DEBUG2
			write_log("ls_zsendfile: EOF at %d",txpos+bread);
#endif
			if(sfStream == mode) frame = ZCRCE;
			else frame = ZCRCW;
		} else {							/* This is not-last sub-frame */
			switch(mode) {
			case sfStream:			frame = ZCRCG; break;	/* Simple sub-frame */
			case sfSlidingWindow:	frame = ZCRCQ; break;	/* Simple sub-frame, but with SlWin */
			case sfBuffered:
				if(txpos + bread > ls_txLastACK + ls_txWinSize) frame = ZCRCW;	/* Last sub-frame in buffer */
				else frame = ZCRCG;	/* Simple sub-frame */
				break;
			}
		}
#ifdef Z_DEBUG2
		write_log("ls_zsendfile: send at %d",txpos);
#endif
		if((rc=ls_zsenddata(txbuf,bread,frame))<0) return rc;
		txpos += bread;

		sendf.foff = txpos;
		check_cps();
		qpfsend();
		/* Ok, now wait for ACKs if here is window, or sample for RPOSes */
		trys = 0;
		do {
			needack = (ZCRCW == frame) || (ls_txWinSize && txpos > ls_txLastACK + ls_txWinSize);
			switch((rc=ls_zrecvhdr(ls_rxHdr,&hlen,needack?ls_HeaderTimeout:0))) {
			case ZSKIP:		/* They don't need this file */
			case ZFERR:		/* Problems occured -- suspend file */
#ifdef Z_DEBUG2
				write_log("ls_zsendfile: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
				return rc;
			case ZACK:		/* Ok, position ACK */
#ifdef Z_DEBUG2
				write_log("ls_zsendfile: ACK at %d",ls_fetchlong(ls_rxHdr));
#endif
				ls_txLastACK = ls_fetchlong(ls_rxHdr);
				break;
			case ZRPOS:		/* Repos */
#ifdef Z_DEBUG2
				write_log("ls_zsendfile: ZRPOS");
#endif
				if((rc=ls_zrpos(ls_fetchlong(ls_rxHdr)))<0) return rc;
				frame = ZCRCW; /* Force to retransmit ZDATA */
				break;
			case ZABORT:	/* Abort transfer */
			case ZFIN:		/* Strange? Ok, abort too */
			case ZCAN:		/* Abort too */
			case LSZ_CAN:
				ls_finishsend();
				/* Fall through */
			case LSZ_RCDO:
			case LSZ_ERROR:
#ifdef Z_DEBUG2
				write_log("ls_zsendfile: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
				return LSZ_ERROR;
			case LSZ_TIMEOUT:	/* Ok! */
				break;
			default:		/* STRANGE! */
#ifdef Z_DEBUG
				write_log("ls_zsendfile: something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
				if(rc < 0) return rc;
				break;
			}
		} while(
			((ls_txWinSize && txpos > ls_txLastACK + ls_txWinSize)	/* Here is window, and we send more than window without ACK*/
			|| ((ZCRCW == frame) && txpos > ls_txLastACK)) 			/* Frame was ZCRCW and here is no ACK for it */
			&& ++trys < 10);										/* trys less than 10 */
		if(trys >= 10) {
#ifdef Z_DEBUG
			write_log("ls_zsendfile: Trys when waiting for ACK/RPOS exceed");
#endif
			return LSZ_ERROR;
		}
		/* Ok, increase block, if here is MANY good blocks was sent */
		if(++ls_txGoodBlocks > 32) {
			ls_txCurBlockSize <<= 1;
			if(ls_txCurBlockSize > ls_MaxBlockSize) ls_txCurBlockSize = ls_MaxBlockSize;
			ls_txGoodBlocks = 0;
		}

		/* Ok, if here is EOF, send it and wait for ZRINIT or ZRPOS */
		/* We do it here, because we coulde receive ZRPOS as answer */
		if(feof(txfd)) {
			ls_storelong(ls_txHdr,txpos);
			if((rc=ls_zsendhhdr(ZEOF,4,ls_txHdr))<0) return rc;
			trys = 0;
			do {
				switch((rc=ls_zrecvhdr(ls_rxHdr,&hlen,ls_HeaderTimeout))) {
				case ZSKIP:		/* They don't need this file */
				case ZFERR:		/* Problems occured -- suspend file */
#ifdef Z_DEBUG2
					write_log("ls_zsendfile: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
					return rc;
				case ZRPOS:		/* Repos */
#ifdef Z_DEBUG2
					write_log("ls_zsendfile: ZRPOS after ZEOF",ls_fetchlong(ls_rxHdr));
#endif
					if((rc=ls_zrpos(ls_fetchlong(ls_rxHdr)))<0) return rc;
					frame = ZCRCW; /* Force to retransmit ZDATA */
					break;
				case ZRINIT:	/* OK! */
					return LSZ_OK;
				case ZACK:		/* ACK for data -- it lost! */
#ifdef Z_DEBUG2
					write_log("ls_zsendfile: ACK after EOF at %d",ls_fetchlong(ls_rxHdr));
#endif
					ls_txLastACK = ls_fetchlong(ls_rxHdr);
				break;
				case ZABORT:	/* Abort transfer */
				case ZFIN:		/* Strange? Ok, abort too */
				case ZCAN:		/* Abort too */
#ifdef Z_DEBUG
					write_log("ls_zsendfile: after ZEOF %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
					return LSZ_ERROR;
				case LSZ_TIMEOUT:	/* Ok, here is no header */
#ifdef Z_DEBUG2
					write_log("ls_zsendfile: TIEMOUT after ZEOF");
#endif
					trys++;
					break;
				default:		/* STRANGE! */
#ifdef Z_DEBUG
					write_log("ls_zsendfile: something strange after ZEOF %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
					if(rc<0) return rc;
					trys++;
					break;
				}
			} while(feof(txfd) && trys < 10);
			if(feof(txfd)) {
#ifdef Z_DEBUG
				write_log("ls_zsendfile: Trys when waiting for ZEOF ACK exceed");
#endif
				return LSZ_ERROR;
			}
		}
	}
#ifdef Z_DEBUG
	write_log("ls_zsendfile: timeout or something else %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
	return rc;
}

/* Done sender -- good way */
int ls_zdonesender()
{
	int rc;
	int hlen;
	int trys = 0;
	int retransmit = 1;
#ifdef Z_DEBUG
	write_log("ls_zdonesender");
#endif
	do {
		if(retransmit) {
			ls_storelong(ls_txHdr,0);
			if((rc=ls_zsendhhdr(ZFIN,4,ls_txHdr))<0) return rc;
			trys++;
			retransmit = 0;
		}
		switch ((rc=ls_zrecvhdr(ls_rxHdr,&hlen,ls_HeaderTimeout))) {
		case ZFIN:				/* Ok, GOOD */
			PUTCHAR('O');PUTCHAR('O');
			return LSZ_OK;
		case ZNAK:
		case LSZ_TIMEOUT:
			retransmit = 1;
			break;
		default:
#ifdef Z_DEBUG
			write_log("ls_zdonesender: something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			if(rc<0) return rc;
			retransmit = 1;
		}
	} while (trys < 10);
#ifdef Z_DEBUG
	write_log("ls_zdonesender: timeout or somethin strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
	return rc;
}
