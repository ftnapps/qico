/**********************************************************
 * File: ls_zglue.c
 * Created at Wed Dec 13 22:52:06 2000 by lev // lev@serebryakov.spb.ru
 *
 * $Id: ls_zglue.c,v 1.2 2000/12/30 19:37:21 lev Exp $
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
	sline("ZSend %s %p",sendas);
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
	sline("ZS: File not found %s!", tosend);
	return OK;
}

/* Init sending -- wrapper */
int zmodem_sendinit(int canzap) {
	int rc;
	if((rc=ls_zinitsender(canzap?LSZ_OPTZEDZAP|LSZ_OPTCRC32|LSZ_OPTSKIPGUARD:LSZ_OPTCRC32|LSZ_OPTSKIPGUARD,effbaud,cfgi(CFG_ZTXWIN),NULL))<0) return rc;
	write_log("zmodem options: %d/%d/%s%s%s%s",ls_MaxBlockSize,ls_txWinSize,
				(ls_Protocol&LSZ_OPTCRC32)?"CRC32":"CRC16",
				(ls_rxCould&LSZ_RXCANDUPLEX)?"/DPX":"",
				(ls_Protocol&LSZ_OPTVHDR)?"/VHDR":"",
                (ls_Protocol&LSZ_OPTESCAPEALL)?"/ESCALL":"");
	return rc;
}

/* Done sending */
int zmodem_senddone()
{
	if(txbuf) sfree(txbuf);
	return ls_zdonesender();
}

int zmodem_receive(char *c) {
	ZFILEINFO f;
	int rc;
	int frame = ZRINIT;

#ifdef Z_DEBUG
	write_log("zmodem_receive");
#endif

	switch((rc=ls_zinitreceiver(LSZ_OPTCRC32|LSZ_OPTZEDZAP|LSZ_OPTSKIPGUARD,effbaud,cfgi(CFG_ZRXWIN),&f))) {
	case ZEOF:
#ifdef Z_DEBUG2
		write_log("zmodem_receive: EOF after INIT");
#endif
		ls_zdonereceiver();
		return LSZ_OK;
	case ZFILE:
		break;
	default:
#ifdef Z_DEBUG
		write_log("zmodem_receive: something strange after init: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]);
#endif
		ls_zabort();
		ls_zdonereceiver();
		return LSZ_ERROR;
	}
	write_log("zmodem options: %d/%d/%s%s%s%s",ls_MaxBlockSize,ls_txWinSize,
				(ls_Protocol&LSZ_OPTCRC32)?"CRC32":"CRC16",
				(ls_rxCould&LSZ_RXCANDUPLEX)?"/DPX":"",
				(ls_Protocol&LSZ_OPTVHDR)?"/VHDR":"",
                (ls_Protocol&LSZ_OPTESCAPEALL)?"/ESCALL":"");
	while(1) {
		switch(rc) {
		case ZFIN:
#ifdef Z_DEBUG2
			write_log("zmodem_receive: EOF");
#endif
			ls_zdonereceiver();
			return LSZ_OK;
		case ZFILE:
			if(-1 == f.filesleft) {
				write_log("zmodem: file %s information bad, skip it",f.name);
				frame = ZSKIP;
			} else {
				switch(rxopen(f.name,f.size,f.mtime,&rxfd)) {
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
						write_log("zmodem_receive: FERR");
#endif
						rxclose(&rxfd,FOP_SUSPEND);
						frame = ZFERR;
						break;
					case ZSKIP:
#ifdef Z_DEBUG2
						write_log("zmodem_receive: FSKIP");
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