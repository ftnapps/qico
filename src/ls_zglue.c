/**********************************************************
 * File: ls_zglue.c
 * Created at Wed Dec 13 22:52:06 2000 by lev // lev@serebryakov.spb.ru
 *
 * $Id: ls_zglue.c,v 1.9 2001/02/18 18:10:54 lev Exp $
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

/* For external use */
int zmodem_sendfile(char *tosend, char *sendas, unsigned long *totalleft, unsigned long *filesleft)
{
	int rc;
	ZFILEINFO f;
#ifdef Z_DEBUG
	write_log("zmodem_sendfile: %s as %s",tosend,sendas);
#endif

	txfd=txopen(tosend,sendas);
	sline("ZSend %s",sendas);
	if(txfd) {
		strcpy(f.name,sendas);
		f.size = sendf.ftot;
		f.mtime = sendf.mtime;
		f.filesleft = *filesleft;
		f.bytesleft = *totalleft;
		rc = ls_zsendfile(&f,ls_SerialNum++);
		(*totalleft)-=sendf.ftot;(*filesleft)--;
#ifdef Z_DEBUG
		write_log("zmodem_sendfile: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
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
#ifdef Z_DEBUG
	write_log("zmodem_sendinit: %s",canzap==2?"DirZap":canzap?"ZedZap":"ZModem");
#endif
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
#ifdef Z_DEBUG
		write_log("zmodem_sendinit: strange canzap: %d",canzap);
#endif
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
	if(txbuf) sfree(txbuf);
#ifdef Z_DEBUG
	write_log("zmodem_senddone");
#endif
	return ls_zdonesender();
}

int zmodem_receive(char *c, int canzap) {
	ZFILEINFO f;
	int rc;
	int frame = ZRINIT;
	int opts = LSZ_OPTCRC32|LSZ_OPTSKIPGUARD|LSZ_OPTZEDZAP;

#ifdef Z_DEBUG
	write_log("zmodem_receive");
#endif
	switch(canzap & 0x00FF) {
	case 2:
		opts |= LSZ_OPTDIRZAP;
		/* Fall through */
	case 1:
	case 0:
		break;
	default:
#ifdef Z_DEBUG
		write_log("zmodem_receive: strange canzap: %d",canzap);
#endif
		break;
	}
	if(canzap & 0x0100) opts |=	LSZ_OPTFIRSTBATCH;

	switch((rc=ls_zinitreceiver(opts,effbaud,cfgi(CFG_ZRXWIN),&f))) {
	case ZFIN:
#ifdef Z_DEBUG2
		write_log("zmodem_receive: ZFIN after INIT, empty batch");
#endif
		ls_zdonereceiver();
		return LSZ_OK;
	case ZFILE:
#ifdef Z_DEBUG2
		write_log("zmodem_receive: ZFILE after INIT");
#endif
		break;
	default:
#ifdef Z_DEBUG
		write_log("zmodem_receive: something strange after init: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
		ls_zabort();
		ls_zdonereceiver();
		return LSZ_ERROR;
	}
	write_log("zmodem link options: %d/%d, %s%s%s%s",ls_MaxBlockSize,ls_txWinSize,
				(ls_Protocol&LSZ_OPTCRC32)?"CRC32":"CRC16",
				(ls_rxCould&LSZ_RXCANDUPLEX)?",DUPLEX":"",
				(ls_Protocol&LSZ_OPTVHDR)?",VHEADER":"",
				(ls_Protocol&LSZ_OPTESCAPEALL)?",ESCALL":"");
	while(1) {
		switch(rc) {
		case ZFIN:
#ifdef Z_DEBUG2
			write_log("zmodem_receive: ZFIN");
#endif
			ls_zdonereceiver();
			return LSZ_OK;
		case ZFILE:
			if(-1 == f.filesleft) {
				write_log("zmodem: file %s information bad, skip it",f.name);
				frame = ZSKIP;
			} else {
				switch(rxopen(f.name,f.mtime,f.size,&rxfd)) {
				case FOP_SKIP:
#ifdef Z_DEBUG2
					write_log("zmodem_receive: SKIP %s",f.name);
#endif
					frame = ZSKIP;
					break;
				case FOP_SUSPEND:
#ifdef Z_DEBUG2
					write_log("zmodem_receive: SUSPEND %s",f.name);
#endif
					frame = ZFERR;
					break;
				case FOP_CONT:
				case FOP_OK:
#ifdef Z_DEBUG2
					write_log("zmodem_receive: OK %s from %d",f.name,recvf.soff);
#endif
					frame = ZRINIT;
					switch((rc=ls_zrecvfile(recvf.soff))) {
					case ZFERR:
#ifdef Z_DEBUG2
						write_log("zmodem_receive: ZFERR");
#endif
						rxclose(&rxfd,FOP_SUSPEND);
						frame = ZFERR;
						break;
					case ZSKIP:
#ifdef Z_DEBUG2
						write_log("zmodem_receive: ZSKIP");
#endif
						rxclose(&rxfd,FOP_SKIP);
						frame = ZSKIP;
						break;
					case LSZ_OK:
#ifdef Z_DEBUG2
						write_log("zmodem_receive: OK");
#endif
						rxclose(&rxfd,FOP_OK);
						break;
					default:
#ifdef Z_DEBUG2
						write_log("zmodem_receive: OTHER %d",rc);
#endif
						rxclose(&rxfd,FOP_ERROR);
						return LSZ_ERROR;
					}
					break;
				}
			}
			break;
		case ZABORT:
#ifdef Z_DEBUG
			write_log("zmodem_receive: ABORT");
#endif
			ls_zabort();
			ls_zdonereceiver();
			return LSZ_ERROR;
		default:
#ifdef Z_DEBUG
			write_log("zmodem_receive: something strange: %d, %s ",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
			ls_zabort();
			ls_zdonereceiver();
			return LSZ_ERROR;
		}
		rc=ls_zrecvfinfo(&f,frame,0);
	}
	return LSZ_OK;
}
