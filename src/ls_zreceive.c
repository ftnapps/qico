/************************************************************
 * File: ls_zreceive.c
 * Created at Sun Dec 17 20:14:03 2000 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: ls_zreceive.c,v 1.11 2001/02/18 18:10:54 lev Exp $
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

static char ls_rxAttnStr[LSZ_MAXATTNLEN+1] = "";

/* Init receiver, preapre to receive files (initialize timeouts too!) */
int ls_zinitreceiver(int protocol, int baud, int window, ZFILEINFO *f)
{

#ifdef Z_DEBUG
	write_log("ls_zinitreceiver: %08x, %d baud, %d bytes",protocol,baud,window);
#endif

	/* Set all options to requested state -- this may be alerted by other side in ZSINIT */
	ls_Protocol = protocol;

	ls_txWinSize = window;

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

	if(NULL==(rxbuf=malloc((ls_MaxBlockSize+16)))) return LSZ_ERROR;

	return ls_zrecvfinfo(f,ZRINIT,(protocol&LSZ_OPTFIRSTBATCH)?0:1);
}

/* Internal function -- receive ZCRCW frame in 10 trys, send ZNAK/ZACK */
int ls_zrecvcrcw(char *buf, int *len)
{
	int trys = 0;
	int rc;

#ifdef Z_DEBUG
	write_log("ls_zrecvcrcw");
#endif
	do {
		switch((rc=ls_zrecvdata(buf,len,ls_DataTimeout,ls_Protocol&LSZ_OPTCRC32))) {
		case ZCRCW:	/* Ok, here it is */
#ifdef Z_DEBUG2
			write_log("ls_zrecvcrcw: got frame ZCRCW");
#endif
			return LSZ_OK;
		case LSZ_BADCRC:
#ifdef Z_DEBUG2
			write_log("ls_zrecvcrcw: BADCRC");
#endif
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			return 1;
		case LSZ_TIMEOUT:
#ifdef Z_DEBUG2
			write_log("ls_zrecvcrcw: TIMEOUT");
#endif
			break;
		default:
#ifdef Z_DEBUG
			write_log("ls_zrecvcrcw:: strange data frame %d",rc);
#endif
			if(rc<0) return rc;
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			return 1;
		}
	} while (++trys < 10);
	ls_storelong(ls_txHdr,0L);
	ls_zsendhhdr(ZNAK,4,ls_txHdr);
	return 1;
}

int ls_zrecvfinfo(ZFILEINFO *f, int frame, int first)
{
	int trys = 0;
	int retransmit = (frame != ZRINIT);
	int hlen = 0;
	int len = 0;
	int rc = 0;
	int zfins = 0;	/* ZFIN counter -- we will trust only MAY of them on first stage */
	char flags;
	int win;

#ifdef Z_DEBUG
	write_log("ls_zrecvfinfo: frame %d, %s, first: %d",frame,LSZ_FRAMETYPES[frame+LSZ_FTOFFSET],first);
#endif
	/* Send temporary variables */
	win = STOI(ls_txWinSize);
	flags = LSZ_RXCANDUPLEX | LSZ_RXCANOVIO;
	if(ls_Protocol&LSZ_OPTCRC32) flags |= LSZ_RXCANFC32;
	if(ls_Protocol&LSZ_OPTESCAPEALL) flags |= LSZ_RXWNTESCCTL;
	if(ls_Protocol&LSZ_OPTESC8) flags |= LSZ_RXWNTESC8;

	f->name[0] = '\x00';
	f->mtime = f->size = f->filesleft = f->bytesleft = 0;

	do {
		if(retransmit) {
			if(ZRINIT != frame) {
#ifdef Z_DEBUG2
				write_log("ls_zrecvfinfo: send %d, %s",frame,LSZ_FRAMETYPES[frame+LSZ_FTOFFSET]);
#endif
				ls_storelong(ls_txHdr,ls_SerialNum);
				if((rc=ls_zsendhhdr(frame,4,ls_txHdr))<0) return rc;
			}
#ifdef Z_DEBUG2
			write_log("ls_zrecvfinfo: send %d, ZRINIT",ZRINIT);
#endif
			ls_txHdr[LSZ_F0] = flags;
			ls_txHdr[LSZ_F1] = (ls_Protocol&LSZ_OPTVHDR)?(char)LSZ_RXCANVHDR:0;
			ls_txHdr[LSZ_P1] = (unsigned char)((win>>8) & 0xff);
			ls_txHdr[LSZ_P0] = (unsigned char)(win & 0xff);
			if((rc=ls_zsendhhdr(ZRINIT,4,ls_txHdr))<0) return rc;
			retransmit = 0;
			trys++;
		}
		switch((rc=ls_zrecvhdr(ls_rxHdr,&hlen,ls_HeaderTimeout))) {
		case ZRQINIT:		/* Send ZRINIT again */
			first = 0;      /* We will trust in first ZFIN after ZRQINIT */
		case ZNAK:
		case LSZ_TIMEOUT:
#ifdef Z_DEBUG2
			write_log("ls_zrecvfinfo: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			retransmit = 1;
			break;
		case ZSINIT:		/* He want to set some options */
#ifdef Z_DEBUG2
			write_log("ls_zrecvfinfo: ZSINIT");
#endif
			first = 0;      /* We will trust in first ZFIN after ZSINIT */
			if((rc=ls_zrecvcrcw(rxbuf,&len))<0) return rc;
			if(!rc) { 		/* Everything is OK */
				ls_storelong(ls_txHdr,1L);
				ls_zsendhhdr(ZACK,4,ls_txHdr);
				strncpy(ls_rxAttnStr,rxbuf,LSZ_MAXATTNLEN);
				ls_rxAttnStr[LSZ_MAXATTNLEN] = '\x00';
				if(ls_rxHdr[LSZ_F0]&LSZ_TXWNTESCCTL) ls_Protocol |= LSZ_OPTESCAPEALL;
				if(ls_rxHdr[LSZ_F0]&LSZ_TXWNTESC8) ls_Protocol |= LSZ_OPTESC8;
			} else {		/* We could not receive ZCRCW subframe, but error is not fatal */
				trys++;
			}
			break;
		case ZFILE:			/* Ok, File started! */
#ifdef Z_DEBUG2
			write_log("ls_zrecvfinfo: ZFILE");
#endif
			if((rc=ls_zrecvcrcw(rxbuf,&len))<0) return rc;
			if(!rc) { 		/* Everything is OK, decode frame */
				strncpy(f->name,rxbuf,MAX_PATH);
				f->name[MAX_PATH-1] = '\x00';
				if(sscanf(rxbuf+strlen(f->name)+1,"%ld %lo %o %o %ld %ld",&f->size,&f->mtime,&len,&ls_SerialNum,&f->filesleft,&f->bytesleft) < 2) {
#ifdef Z_DEBUG
					write_log("ls_zrecvfinfo: file info is corrupted: '%s'",rxbuf+strlen(f->name)+1);
#endif
					f->filesleft = -1;
				}
				return ZFILE;
			} else {		/* We could not receive ZCRCW subframe, but error is not fatal */
				trys++;
			}
			break;
		case ZFIN:			/* ZFIN from previous session? Or may be real one? */
#ifdef Z_DEBUG2
			write_log("ls_zrecvfinfo: ZFIN %d, first: %d",zfins,first);
#endif
			if(!first || ++zfins == LSZ_TRUSTZFINS) return ZFIN;
			break;
		case ZABORT:		/* Abort this session -- we trust in ABORT! */
#ifdef Z_DEBUG2
			write_log("ls_zrecvfinfo: ABORT");
#endif
			return ZABORT;
		case LSZ_BADCRC:
#ifdef Z_DEBUG2
			write_log("ls_zrecvfinfo: BADCRC");
#endif
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			retransmit = 1;
			break;
		default:
#ifdef Z_DEBUG
			write_log("ls_zrecvfinfo: something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			if(rc<0) return rc;
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			break;
		}
	} while(trys < 10);
#ifdef Z_DEBUG
	write_log("ls_zrecvfinfo: timeout or something other: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
	return LSZ_TIMEOUT;
}

/* Receive ZDATA/ZEOF frame, do 10 trys, return position */
int ls_zrecvnewpos(unsigned long oldpos, unsigned long *pos)
{
	int rc = 0;
	int trys = 0;
	int hlen;

#ifdef Z_DEBUG
	write_log("ls_zrecvnewpos: will retransmit %d",oldpos);
#endif
	do {
		switch((rc=ls_zrecvhdr(ls_rxHdr,&hlen,ls_HeaderTimeout))) {
		case ZDATA:
		case ZEOF:
#ifdef Z_DEBUG2
			write_log("ls_zrecvnewpos: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			*pos = ls_fetchlong(ls_rxHdr);
			return rc;
		case ZNAK:
#ifdef Z_DEBUG2
			write_log("ls_zrecvnewpos: ZNAK");
#endif
			ls_storelong(ls_txHdr,oldpos);
			if((rc=ls_zsendhhdr(ZRPOS,4,ls_txHdr))<0) return rc;
			break;
		case LSZ_TIMEOUT:
#ifdef Z_DEBUG2
			write_log("ls_zrecvnewpos: TIEMOUT");
#endif
			break;
		case LSZ_BADCRC:
#ifdef Z_DEBUG2
			write_log("ls_zrecvnewpos: BADCRC");
#endif
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			break;
		default:
#ifdef Z_DEBUG
			write_log("ls_zrecvnewpos: something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			if(rc<0) return rc;
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			break;
		}
	} while (++trys < 10);
#ifdef Z_DEBUG
	write_log("ls_zrecvnewpos: timeout or something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
	return LSZ_TIMEOUT;
}

/* Receive one file */
int ls_zrecvfile(int pos)
{
	unsigned long newpos;
	int rc;
	int needzdata = 1;
	int len;

#ifdef Z_DEBUG
	write_log("ls_zrecvfile form %d",pos);
#endif

	rxpos = pos;
	ls_storelong(ls_txHdr,rxpos);
	if((rc=ls_zsendhhdr(ZRPOS,4,ls_txHdr))<0) return rc;

	do {
		if(!needzdata) {
			switch((rc=ls_zrecvdata(rxbuf,&len,ls_DataTimeout,ls_Protocol&LSZ_OPTCRC32))) {
			case ZCRCE:
				needzdata = 1;
			case ZCRCG:
#ifdef Z_DEBUG2
				write_log("ls_zrecvfile: ZCRC%c, %d bytes at %d",rc==ZCRCE?'E':'G',len,rxpos);
#endif
				rxpos += len;
				if(len != fwrite(rxbuf,1,len,rxfd)) return ZFERR;

				recvf.foff = rxpos;
				check_cps();
				qpfrecv();
				break;
			case ZCRCW:
				needzdata = 1;
			case ZCRCQ:
#ifdef Z_DEBUG2
				write_log("ls_zrecvfile: ZCRC%c, %d bytes at %d",rc==ZCRCW?'W':'Q',len,rxpos);
#endif
				rxpos += len;
				if(len != fwrite(rxbuf,1,len,rxfd)) return ZFERR;
				ls_storelong(ls_txHdr,rxpos);
				ls_zsendhhdr(ZACK,4,ls_txHdr);

				recvf.foff = rxpos;
				check_cps();
				qpfrecv();
				break;
			case LSZ_BADCRC:
			case LSZ_TIMEOUT:
#ifdef Z_DEBUG
				write_log("ls_zrecvfile: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
				if(ls_rxAttnStr[0]) PUTSTR(ls_rxAttnStr);
				PURGE();
				ls_storelong(ls_txHdr,rxpos);
				ls_zsendhhdr(ZRPOS,4,ls_txHdr);
				needzdata = 1;
				break;
			default:
#ifdef Z_DEBUG
				write_log("ls_zrecvfile: something strange %d",rc);
#endif
				if(rc<0) return rc;
				if(ls_rxAttnStr[0]) PUTSTR(ls_rxAttnStr);
				PURGE();
				ls_storelong(ls_txHdr,rxpos);
				ls_zsendhhdr(ZRPOS,4,ls_txHdr);
				needzdata = 1;
			}
		} else {		/* We need new position -- ZDATA (and may be ZEOF) */
#ifdef Z_DEBUG
			write_log("ls_zrecvfile: want ZDATA/ZEOF at %d",rxpos);
#endif
			if((rc=ls_zrecvnewpos(rxpos,&newpos))<0) return rc;
			if(newpos != rxpos) {
#ifdef Z_DEBUG
				write_log("ls_zrecvfile: bad new position %d in %s",newpos,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
				if(ls_rxAttnStr[0]) PUTSTR(ls_rxAttnStr);
				PURGE();
				ls_storelong(ls_txHdr,rxpos);
				if((rc=ls_zsendhhdr(ZRPOS,4,ls_txHdr))<0) return rc;
			} else {
				if(ZEOF == rc) {
#ifdef Z_DEBUG2
					write_log("ls_zrecvfile: ZEOF");
#endif
					ls_storelong(ls_txHdr,0);
					if((rc=ls_zsendhhdr(ZRINIT,4,ls_txHdr))<0) return rc;
					return LSZ_OK;
				}
#ifdef Z_DEBUG2
				write_log("ls_zrecvfile: ZDATA");
#endif
				needzdata = 0;
			}
		}
	} while(1);
	return LSZ_OK;
}

int ls_zdonereceiver()
{
	int rc;
	int trys = 0;
	int hlen;
	int retransmit = 1;
#ifdef Z_DEBUG
	write_log("ls_zdonereceiver");
#endif
	do {
		if (retransmit) {
			ls_storelong(ls_txHdr,0);
			if((rc=ls_zsendhhdr(ZFIN,4,ls_txHdr))<0) return rc;
			retransmit = 0;
			trys++;
		}
		switch (rc=GETCHAR(ls_HeaderTimeout)) {
		case 'O':				/* Ok, GOOD */
#ifdef Z_DEBUG2
			write_log("ls_zdonereceiver: O");
#endif
			rc = GETCHAR(0);
			return LSZ_OK;
		case XON: case XOFF:
		case XON | 0x80: case XOFF | 0x80:
#ifdef Z_DEBUG2
			write_log("ls_zdonereceiver: XON/XOFF, skip it");
#endif
			break;
		case ZPAD:
#ifdef Z_DEBUG2
			write_log("ls_zdonereceiver: ZPAD");
#endif
			if((rc=ls_zrecvhdr(ls_rxHdr,&hlen,ls_HeaderTimeout))<0) return rc;
			if(ZFIN != rc) return LSZ_OK;
			retransmit = 1;
			break;
		default:
#ifdef Z_DEBUG
			write_log("ls_zdonereceiver: something strange %d",rc);
#endif
			if(rc<0) return rc;
			retransmit = 1;
			break;
		}
	} while (trys < 10);
#ifdef Z_DEBUG
	write_log("ls_zdonereceiver: timeout or something strange %d",rc);
#endif
	return rc;
}
