/*
   ZModem file transfer protocol. Written from scratches.
   Support CRC16, CRC32, variable header, ZedZap (big blocks) and DirZap.
   Global variables, common functions.
   $Id: ls_zglue.c,v 1.1.1.1 2003/07/12 21:26:46 sisoft Exp $
*/
#include "headers.h"
#include "defs.h"
#include "ls_zmodem.h"

/* For external use */
int zmodem_sendfile(char *tosend, char *sendas, unsigned long *totalleft, unsigned long *filesleft)
{
	int rc;
	ZFILEINFO f;
	DEBUG(('Z',1,"zmodem_sendfile: %s as %s",tosend,sendas));

	txfd=txopen(tosend,sendas);
	sline("ZSend %s",sendas);
	if(txfd) {
		xstrcpy(f.name,sendas,MAX_PATH);
		f.size = sendf.ftot;
		f.mtime = sendf.mtime;
		f.filesleft = *filesleft;
		f.bytesleft = *totalleft;
		rc = ls_zsendfile(&f,ls_SerialNum++);
		(*totalleft)-=sendf.ftot;(*filesleft)--;
		DEBUG(('Z',1,"zmodem_sendfile: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
		switch(rc) {
		case LSZ_OK:
			txclose(&txfd,FOP_OK);
			break;
		case ZSKIP:
			txclose(&txfd,FOP_SKIP);
			break;
		case ZFERR:
			txclose(&txfd,FOP_SUSPEND);
			break;
		default:
			txclose(&txfd,FOP_ERROR);
			ls_zabort();
			break;
		}
		return rc;
	}
	sline("ZSend: File %s not found!", tosend);
	return OK;
}

/* Init sending -- wrapper */
int zmodem_sendinit(int canzap) {
	int rc;
	int opts = LSZ_OPTCRC32|LSZ_OPTSKIPGUARD;
	DEBUG(('Z',1,"zmodem_sendinit: %s",canzap==2?"DirZap":canzap?"ZedZap":"ZModem"));
	switch(canzap) {
	case 2:
		opts |= LSZ_OPTDIRZAP;
		/* Fall through */
	case 1:
		opts |= LSZ_OPTZEDZAP;
		/* Fall through */
	case 0:
		break;
	default:
		DEBUG(('Z',1,"zmodem_sendinit: strange canzap: %d",canzap));
		break;
	}
	if((rc=ls_zinitsender(opts,effbaud,cfgi(CFG_ZTXWIN),NULL))<0) return rc;
	write_log("zmodem link options: %d/%d, %s%s%s%s",ls_MaxBlockSize,ls_txWinSize,
				(ls_Protocol&LSZ_OPTCRC32)?"CRC32":"CRC16",
				(ls_rxCould&LSZ_RXCANDUPLEX)?",DUPLEX":"",
				(ls_Protocol&LSZ_OPTVHDR)?",VHEADER":"",
				(ls_Protocol&LSZ_OPTESCAPEALL)?",ESCALL":"");
	return rc;
}

/* Done sending */
int zmodem_senddone()
{
	DEBUG(('Z',1,"zmodem_senddone"));
	xfree(txbuf);
	return ls_zdonesender();
}

int zmodem_receive(char *c, int canzap) {
	ZFILEINFO f;
	int rc;
	int frame = ZRINIT;
	int opts = LSZ_OPTCRC32|LSZ_OPTSKIPGUARD;

	DEBUG(('Z',1,"zmodem_receive"));
	switch(canzap & 0x00FF) {
	case 2:
		opts |= LSZ_OPTDIRZAP;
		/* Fall through */
	case 1:
		opts |= LSZ_OPTZEDZAP;
		/* Fall through */
	case 0:
		break;
	default:
		DEBUG(('Z',1,"zmodem_receive: strange canzap: %d",canzap));
		break;
	}
	if(canzap & 0x0100) opts |=	LSZ_OPTFIRSTBATCH;

	switch((rc=ls_zinitreceiver(opts,effbaud,cfgi(CFG_ZRXWIN),&f))) {
	case ZFIN:
		DEBUG(('Z',2,"zmodem_receive: ZFIN after INIT, empty batch"));
		ls_zdonereceiver();
		return LSZ_OK;
	case ZFILE:
		DEBUG(('Z',2,"zmodem_receive: ZFILE after INIT"));
		break;
	default:
		DEBUG(('Z',1,"zmodem_receive: something strange after init: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
		ls_zabort();
		ls_zdonereceiver();
		return LSZ_ERROR;
	}
	write_log("zmodem link options: %d/%d, %s%s%s%s",ls_MaxBlockSize,ls_txWinSize,
				(ls_Protocol&LSZ_OPTCRC32)?"CRC32":"CRC16",
				",DUPLEX",
				(ls_Protocol&LSZ_OPTVHDR)?",VHEADER":"",
				(ls_Protocol&LSZ_OPTESCAPEALL)?",ESCALL":"");
	while(1) {
		switch(rc) {
		case ZFIN:
			DEBUG(('Z',2,"zmodem_receive: ZFIN"));
			ls_zdonereceiver();
			return LSZ_OK;
		case ZFILE:
			if(-1 == f.filesleft) {
				write_log("zmodem: file %s information bad, skip it",f.name);
				frame = ZSKIP;
			} else {
				switch(rxopen(f.name,f.mtime,f.size,&rxfd)) {
				case FOP_SKIP:
					DEBUG(('Z',2,"zmodem_receive: SKIP %s",f.name));
					frame = ZSKIP;
					break;
				case FOP_SUSPEND:
					DEBUG(('Z',2,"zmodem_receive: SUSPEND %s",f.name));
					frame = ZFERR;
					break;
				case FOP_CONT:
				case FOP_OK:
					DEBUG(('Z',2,"zmodem_receive: OK %s from %d",f.name,recvf.soff));
					frame = ZRINIT;
					switch((rc=ls_zrecvfile(recvf.soff))) {
					case ZFERR:
						DEBUG(('Z',2,"zmodem_receive: ZFERR"));
						rxclose(&rxfd,FOP_SUSPEND);
						frame = ZFERR;
						break;
					case ZSKIP:
						DEBUG(('Z',2,"zmodem_receive: ZSKIP"));
						rxclose(&rxfd,FOP_SKIP);
						frame = ZSKIP;
						break;
					case LSZ_OK:
						DEBUG(('Z',2,"zmodem_receive: OK"));
						rxclose(&rxfd,FOP_OK);
						break;
					default:
						DEBUG(('Z',1,"zmodem_receive: OTHER %d",rc));
						rxclose(&rxfd,FOP_ERROR);
						return LSZ_ERROR;
					}
					break;
				}
			}
			break;
		case ZABORT:
			DEBUG(('Z',1,"zmodem_receive: ABORT"));
			ls_zabort();
			ls_zdonereceiver();
			return LSZ_ERROR;
		default:
			DEBUG(('Z',1,"zmodem_receive: something strange: %d, %s ",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
			ls_zabort();
			ls_zdonereceiver();
			return LSZ_ERROR;
		}
		rc=ls_zrecvfinfo(&f,frame,1);
	}
	return LSZ_OK;
}
