/**********************************************************
 * File: ls_zmodem.c
 * Created at Sun Oct 29 18:51:46 2000 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: ls_zmodem.c,v 1.1 2000/11/02 07:00:20 lev Exp $
 **********************************************************/
/*

   ZModem file transfer protocol. Written from scratches.
   Support CRC16, CRC32, RLE Encoding, variable header, ZedZap (big blocks).
   Global variables, common functions.

*/
#include "mailer.h"
#include "defs.h"
#include "ls_zmodem.h"
#include "qipc.h"

/* Common variables */
int ls_UseVarHeaders;		/* Use variable length headers */
int ls_Hdr[MAX_ZHEADER];	/* Received or sended header */
int ls_CRC32;				/* Use CRC32 */

/* Variables to control sender */
int ls_txWinSize;			/* Receiver Window/Buffer size (0 for streaming) */
int ls_txCouldDuplex;		/* Receiver could fullduplex -- use ZCRCQ */
int ls_txLastACK;			/* Last ACKed byte */
int ls_txLastRepos;			/* Last requested byte */
int ls_txCurBlockSize;		/* Current block size */
int ls_txLastSent;			/* Last sent character -- for escaping */
int ls_txCANCount;			/* Count of CANs to go */
int ls_txSendRLE;			/* Receiver want to use RLE coding */
int ls_txEscapeAll;			/* Receiver want us to escape all control chars */
int ls_txZedZap;			/* Big blocks OK */
int ls_txDirZap;			/* Escape only <DLE> */

/* Variables to control receiver */


/* Special table to FAST calculate header type */
/*          CRC32,VAR,RLE */
int HEADER_TYPE[2][2][2] = {{{ZBIN,-1},{ZVBIN,-1}},
							{{ZBIN32,ZBINR32},{ZVBIN32,ZVBINR32}}};

/* Hex digits */
char HEX_DIGITS[] = "0123456789abcdef";

/* Functions */

/* Send binary header. Use proper CRC, send var. len. if could */
int zsendbhdr(int frametype, int len, char *hdr)
{
	int crc = ls_CRC32?LSZINIT_CRC32:LSZINIT_CRC16;
	int type;
	int n;

	/* First, calculate packet header byte */
	if ((type = HEADER_TYPE[ls_CRC32][ls_UseVarHeaders][ls_txSendRLE]) < 0) {
		log("zmodem: invalid options set, %s, %s, %s",
			ls_CRC32?"crc32":"crc16",
			ls_UseVarHeaders?"varh":"fixh",
			ls_txSendRLE?"RLE":"Plain");
		return ERROR;
	}

	/* Send *<DLE> and packet type */
	BUFCHAR(ZPAD);BUFCHAR(ZDLE);BUFCHAR(type);
	if (ls_UseVarHeaders) ls_sendchar(len);			/* Send length of header, if needed */

	ls_sendchar(frametype);							/* Send type of frame */
	crc = LS_UPDATE_CRC(frametype,crc);
	/* Send whole header */
	for (n=len; --n >= 0; ++hdr) {
		ls_sendhex(*hdr);
		crc = LS_UPDATE_CRC16((0xFF & *hdr),crc);
	}
	if (ls_CRC32) {
		crc = ~crc;
		crc = LTOI(crc);
		for (n=4; --n >= 0;) {
			ls_sendchar(crc & 0xFF);
			crc >>= 8;
		}
	} else {
		crc = LS_UPDATE_CRC(0,LS_UPDATE_CRC(0,crc));
		crc &= 0xFFFF;
		crc = STOI(crc);
		ls_sendchar(crc >> 8);
		ls_sendchar(crc & 0xFF);
	}
	/* Clean buffer, do real send */
	return BUFLUSH();
}

/* Send HEX header. Use CRC16, send var. len. if could */
int zsendhhdr(int frametype, int len, char *hdr)
{
	int crc = LSZINIT_CRC16;

	/* Send **<DLE> */
	BUFCHAR(ZPAD); BUFCHAR(ZPAD); BUFCHAR(ZDLE);

	/* Send header type */
	if (ls_UseVarHeaders) {
		BUFCHAR(ZVHEX);
		ls_sendhex(len);
	} else {
		BUFCHAR(ZHEX);
	}

	ls_sendhex(frametype);
	crc = LS_UPDATE_CRC16(frametype,crc);
	/* Send whole header */
	for (n=len; --n >= 0; ++hdr) {
		ls_sendchar(*hdr);
		crc = LS_UPDATE_CRC16((0xFF & *hdr),crc);
	}
	crc = LS_UPDATE_CRC16(0,LS_UPDATE_CRC16(0,crc));
	crc &= 0xFFFF;
	crc = STOI(crc);
	ls_sendhex(crc >> 8);
	ls_sendhex(crc & 0xFF);
	/* Clean buffer, do real send */
	return BUFLUSH();
}

int ls_zrecvhdr(char *hdr, int timeout)
{
	enum rhSTATE {
		rhInit,				/* Start state */
		rhZPAD,				/* ZPAD got (*) */
		rhZDLE,				/* We got ZDLE */
		rhZBIN,				
	} state = rhInit;

	int frametype = ERROR;
	int ;
}

/* Send one char with escaping */
void ls_sendchar(int i) 
{
	char c = (char)(i & 0xFF);
	int esc = 0;
	if (ls_txDirZap) {	/* We are Direct ZedZap -- escape only <DLE> */
		esc = (ZDLE == c);
	} else {			/* We are normal ZModem */
		if (ls_txEscapeAll && ((c & 0x60) == 0)) { /* Receiver want to escape ALL */
			esc = 1;
		} else {
			switch (c) {
			case XON: case XON | 0x80:
			case XOFF: case XOFF | 0x80: 
			case DLE: case DLE | 0x80:
			case ZDLE:
				esc = 1;
				break;
			default:
				esc = (((ls_txLastSent & 0x7F) == (char)'@') && ((c & 0x7F) == CR));
				break; 
			}
		}
	}
	if (esc) {
		BUFCHAR(ZDLE);
		c |= 0x40;
	}
	BUFCHAR(ls_txLastSent = c);
}

/* Send one char as two hex digits */
void ls_sendhex(int i) 
{
	char c = (char)(i & 0xFF);
	BUFCHAR(HEX_DIGITS[(c & 0xF0) >> 4]);
	BUFCHAR(ls_txLastSent = HEX_DIGITS[c & 0xF]);
}

/* Retrun 7bit character, strip XON/XOFF if not DirZap, with timeout */
int ls_read7bit(int timeout)
{
	int c;

	do {
		if((c = GETCHAR(timeout)) < 0) return c;
	} while((!ls_DirZap) && (XON == c || XOFF = c));

	if (CAN == c) { if (++ls_txCANCount == 5) return GCOUNT; }
	else { ls_txCANCount = 0; }
	return c & 0x7F;
}