/*
   ZModem file transfer protocol. Written from scratches.
   Support CRC16, CRC32, variable header, ZedZap (big blocks) and DirZap.
   Receiver logic.
   $Id: ls_zreceive.c,v 1.1.1.1 2003/07/12 21:26:55 sisoft Exp $
*/
#include "headers.h"
#include "defs.h"
#include "ls_zmodem.h"

static unsigned char ls_rxAttnStr[LSZ_MAXATTNLEN+1] = "";

/* Init receiver, preapre to receive files (initialize timeouts too!) */
int ls_zinitreceiver(int protocol, int baud, int window, ZFILEINFO *f)
{

	DEBUG(('Z',1,"ls_zinitreceiver: %08x, %d baud, %d bytes",protocol,baud,window));

	/* Set all options to requested state -- this may be alerted by other side in ZSINIT */
	ls_Protocol = protocol;

	ls_txWinSize = window;

	/* Maximum block size -- by protocol, may be reduced by window size later */
	ls_MaxBlockSize = ls_Protocol&LSZ_OPTZEDZAP?8192:1024;

	/* Calculate timeouts */
	/* Timeout for header waiting, if no data sent -- 3*TransferTime or 10 seconds */
	ls_HeaderTimeout = (LSZ_MAXHLEN * 30) / baud;
	ls_HeaderTimeout = ls_HeaderTimeout>10?ls_HeaderTimeout:10;
	/* Timeout for data packet (3*TransferTime or 60 seconds) */
	ls_DataTimeout = (ls_MaxBlockSize * 30) / baud;
	ls_DataTimeout = ls_DataTimeout>60?ls_DataTimeout:60;

	ls_SkipGuard = (ls_Protocol&LSZ_OPTSKIPGUARD)?1:0;

	if(NULL==(rxbuf=malloc((ls_MaxBlockSize+16)))) return LSZ_ERROR;

	return ls_zrecvfinfo(f,ZRINIT,(protocol&LSZ_OPTFIRSTBATCH)?1:0);
}

/* Internal function -- receive ZCRCW frame in 10 trys, send ZNAK/ZACK */
int ls_zrecvcrcw(byte *buf, int *len)
{
	int trys = 0;
	int rc;

	DEBUG(('Z',1,"ls_zrecvcrcw"));
	do {
		switch((rc=ls_zrecvdata(buf,len,ls_DataTimeout,ls_Protocol&LSZ_OPTCRC32))) {
		case ZCRCW:	/* Ok, here it is */
			DEBUG(('Z',2,"ls_zrecvcrcw: got frame ZCRCW"));
			return LSZ_OK;
		case LSZ_BADCRC:
			DEBUG(('Z',2,"ls_zrecvcrcw: BADCRC"));
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			return 1;
		case LSZ_TIMEOUT:
			DEBUG(('Z',2,"ls_zrecvcrcw: TIMEOUT"));
			break;
		default:
			DEBUG(('Z',1,"ls_zrecvcrcw:: strange data frame %d",rc));
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

	DEBUG(('Z',1,"ls_zrecvfinfo: frame %d, %s, first: %d",frame,LSZ_FRAMETYPES[frame+LSZ_FTOFFSET],first));
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
				DEBUG(('Z',2,"ls_zrecvfinfo: send %d, %s",frame,LSZ_FRAMETYPES[frame+LSZ_FTOFFSET]));
				ls_storelong(ls_txHdr,ls_SerialNum);
				if((rc=ls_zsendhhdr(frame,4,ls_txHdr))<0) return rc;
			}
			DEBUG(('Z',2,"ls_zrecvfinfo: send %d, ZRINIT",ZRINIT));
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
			first = 1;      /* We will trust in first ZFIN after ZRQINIT */
		case ZNAK:
		case LSZ_TIMEOUT:
			DEBUG(('Z',2,"ls_zrecvfinfo: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
			retransmit = 1;
			break;
		case ZSINIT:		/* He want to set some options */
			DEBUG(('Z',2,"ls_zrecvfinfo: ZSINIT"));
			first = 0;      /* We will trust in first ZFIN after ZSINIT */
			if((rc=ls_zrecvcrcw(rxbuf,&len))<0) return rc;
			if(!rc) { 		/* Everything is OK */
				ls_storelong(ls_txHdr,1L);
				ls_zsendhhdr(ZACK,4,ls_txHdr);
				xstrcpy((char *)ls_rxAttnStr,(char *)rxbuf,LSZ_MAXATTNLEN+1);
				ls_rxAttnStr[LSZ_MAXATTNLEN] = '\x00';
				if(ls_rxHdr[LSZ_F0]&LSZ_TXWNTESCCTL) ls_Protocol |= LSZ_OPTESCAPEALL;
				if(ls_rxHdr[LSZ_F0]&LSZ_TXWNTESC8) ls_Protocol |= LSZ_OPTESC8;
			} else {		/* We could not receive ZCRCW subframe, but error is not fatal */
				trys++;
			}
			break;
		case ZFILE:			/* Ok, File started! */
			DEBUG(('Z',2,"ls_zrecvfinfo: ZFILE"));
			if((rc=ls_zrecvcrcw(rxbuf,&len))<0) return rc;
			if(!rc) { 		/* Everything is OK, decode frame */
				xstrcpy(f->name,(char *)rxbuf,MAX_PATH);
				f->name[MAX_PATH-1] = '\x00';
				if(sscanf((char *)rxbuf+strlen(f->name)+1,"%ld %lo %o %o %ld %ld",&f->size,&f->mtime,&len,&ls_SerialNum,&f->filesleft,&f->bytesleft) < 2) {
					DEBUG(('Z',1,"ls_zrecvfinfo: file info is corrupted: '%s'",rxbuf+strlen(f->name)+1));
					f->filesleft = -1;
				}
				return ZFILE;
			} else {		/* We could not receive ZCRCW subframe, but error is not fatal */
				trys++;
			}
			break;
		case ZFIN:			/* ZFIN from previous session? Or may be real one? */
			DEBUG(('Z',2,"ls_zrecvfinfo: ZFIN %d, first: %d",zfins,first));
			if(first || ++zfins == LSZ_TRUSTZFINS) return ZFIN;
			break;
		case ZABORT:		/* Abort this session -- we trust in ABORT! */
			DEBUG(('Z',2,"ls_zrecvfinfo: ABORT"));
			return ZABORT;
		case LSZ_BADCRC:
			DEBUG(('Z',2,"ls_zrecvfinfo: BADCRC"));
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			retransmit = 1;
			break;
		default:
			DEBUG(('Z',1,"ls_zrecvfinfo: something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
			if(rc<0) return rc;
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			break;
		}
	} while(trys < 10);
	DEBUG(('Z',1,"ls_zrecvfinfo: timeout or something other: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
	return LSZ_TIMEOUT;
}

/* Receive ZDATA/ZEOF frame, do 10 trys, return position */
int ls_zrecvnewpos(unsigned long oldpos, unsigned long *pos)
{
	int rc = 0;
	int trys = 0;
	int hlen;

	DEBUG(('Z',1,"ls_zrecvnewpos: will retransmit %d",oldpos));
	do {
		switch((rc=ls_zrecvhdr(ls_rxHdr,&hlen,ls_HeaderTimeout))) {
		case ZDATA:
		case ZEOF:
			DEBUG(('Z',2,"ls_zrecvnewpos: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
			*pos = ls_fetchlong(ls_rxHdr);
			return rc;
		case ZNAK:
			DEBUG(('Z',2,"ls_zrecvnewpos: ZNAK"));
			ls_storelong(ls_txHdr,oldpos);
			if((rc=ls_zsendhhdr(ZRPOS,4,ls_txHdr))<0) return rc;
			break;
		case LSZ_TIMEOUT:
			DEBUG(('Z',2,"ls_zrecvnewpos: TIEMOUT"));
			break;
		case LSZ_BADCRC:
			DEBUG(('Z',2,"ls_zrecvnewpos: BADCRC"));
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			break;
		default:
			DEBUG(('Z',1,"ls_zrecvnewpos: something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
			if(rc<0) return rc;
			ls_storelong(ls_txHdr,0L);
			ls_zsendhhdr(ZNAK,4,ls_txHdr);
			break;
		}
	} while (++trys < 10);
	DEBUG(('Z',1,"ls_zrecvnewpos: timeout or something strange %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
	return LSZ_TIMEOUT;
}

/* Receive one file */
int ls_zrecvfile(int pos)
{
	unsigned long newpos;
	int rc;
	int needzdata = 1;
	int len;

	DEBUG(('Z',1,"ls_zrecvfile form %d",pos));

	rxpos = pos;
	rxstatus=0;
	ls_storelong(ls_txHdr,rxpos);
	if((rc=ls_zsendhhdr(ZRPOS,4,ls_txHdr))<0) return rc;

	do {
		if(!needzdata) {
			switch((rc=ls_zrecvdata(rxbuf,&len,ls_DataTimeout,ls_Protocol&LSZ_OPTCRC32))) {
			case ZCRCE:
				needzdata = 1;
			case ZCRCG:
				DEBUG(('Z',2,"ls_zrecvfile: ZCRC%c, %d bytes at %d",rc==ZCRCE?'E':'G',len,rxpos));
				rxpos += len;
				if(len != fwrite(rxbuf,1,len,rxfd)) return ZFERR;

				recvf.foff = rxpos;
				check_cps();
				qpfrecv();
				break;
			case ZCRCW:
				needzdata = 1;
			case ZCRCQ:
				DEBUG(('Z',2,"ls_zrecvfile: ZCRC%c, %d bytes at %d",rc==ZCRCW?'W':'Q',len,rxpos));
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
				DEBUG(('Z',1,"ls_zrecvfile: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
				if(ls_rxAttnStr[0]) PUTSTR(ls_rxAttnStr);
				PURGE();
				ls_storelong(ls_txHdr,rxpos);
				ls_zsendhhdr(ZRPOS,4,ls_txHdr);
				needzdata = 1;
				break;
			default:
				DEBUG(('Z',1,"ls_zrecvfile: something strange %d",rc));
				if(rc<0) return rc;
				if(ls_rxAttnStr[0]) PUTSTR(ls_rxAttnStr);
				PURGE();
				ls_storelong(ls_txHdr,rxpos);
				ls_zsendhhdr(ZRPOS,4,ls_txHdr);
				needzdata = 1;
			}
		} else {		/* We need new position -- ZDATA (and may be ZEOF) */
			DEBUG(('Z',1,"ls_zrecvfile: want ZDATA/ZEOF at %d",rxpos));
			if((rc=ls_zrecvnewpos(rxpos,&newpos))<0) return rc;
			if(newpos != rxpos) {
				DEBUG(('Z',1,"ls_zrecvfile: bad new position %d in %s",newpos,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
				if(ls_rxAttnStr[0]) PUTSTR(ls_rxAttnStr);
				PURGE();
				ls_storelong(ls_txHdr,rxpos);
				if((rc=ls_zsendhhdr(ZRPOS,4,ls_txHdr))<0) return rc;
			} else {
				if(ZEOF == rc) {
					DEBUG(('Z',2,"ls_zrecvfile: ZEOF"));
					ls_storelong(ls_txHdr,0);
					if((rc=ls_zsendhhdr(ZRINIT,4,ls_txHdr))<0) return rc;
					return LSZ_OK;
				}
				DEBUG(('Z',2,"ls_zrecvfile: ZDATA"));
				needzdata = 0;
			}
		}
		if(rxstatus)return((rxstatus==RX_SKIP)?ZSKIP:ZFERR);
	} while(1);
	return LSZ_OK;
}

int ls_zdonereceiver()
{
	int rc;
	int trys = 0;
	int hlen;
	int retransmit = 1;
	DEBUG(('Z',1,"ls_zdonereceiver"));
	xfree(rxbuf);
	do {
		if (retransmit) {
			ls_storelong(ls_txHdr,0);
			if((rc=ls_zsendhhdr(ZFIN,4,ls_txHdr))<0) return rc;
			retransmit = 0;
			trys++;
		}
		switch (rc=GETCHAR(ls_HeaderTimeout)) {
		case 'O':				/* Ok, GOOD */
			DEBUG(('Z',2,"ls_zdonereceiver: O"));
			rc = GETCHAR(0);
			return LSZ_OK;
		case XON: case XOFF:
		case XON | 0x80: case XOFF | 0x80:
			DEBUG(('Z',2,"ls_zdonereceiver: XON/XOFF, skip it"));
			break;
		case ZPAD:
			DEBUG(('Z',2,"ls_zdonereceiver: ZPAD"));
			if((rc=ls_zrecvhdr(ls_rxHdr,&hlen,ls_HeaderTimeout))<0) return rc;
			if(ZFIN != rc) return LSZ_OK;
			retransmit = 1;
			break;
		default:
			DEBUG(('Z',1,"ls_zdonereceiver: something strange %d",rc));
			if(rc<0) return rc;
			retransmit = 1;
			break;
		}
	} while (trys < 10);
	DEBUG(('Z',1,"ls_zdonereceiver: timeout or something strange %d",rc));
	return rc;
}
