/**********************************************************
 * File: ls_zmodem.c
 * Created at Sun Oct 29 18:51:46 2000 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: ls_zmodem.c,v 1.2 2000/11/03 07:55:29 lev Exp $
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
int ls_Hdr[LSZ_MAXHLEN];	/* Received or sended header */
int ls_GotZDLE;				/* We seen DLE as last character */
int ls_GotHexNibble;		/* We seen one hex digit as last character */
int ls_Protocol;			/* Plain/ZedZap/DirZap */
int ls_CANCount;			/* Count of CANs to go */
int ls_Garbage;				/* Count of garbage characters */

/* Variables to control sender */
int ls_txWinSize;			/* Receiver Window/Buffer size (0 for streaming) */
int ls_txCouldDuplex;		/* Receiver could fullduplex -- use ZCRCQ */
int ls_txLastACK;			/* Last ACKed byte */
int ls_txLastRepos;			/* Last requested byte */
int ls_txCurBlockSize;		/* Current block size */
int ls_txLastSent;			/* Last sent character -- for escaping */
int ls_txSendRLE;			/* Receiver want to use RLE coding */
int ls_txEscapeAll;			/* Receiver want us to escape all control chars */
int ls_txCRC32;				/* Use CRC32 */

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
	int crc = ls_txCRC32?LSZ_INIT_CRC32:LSZ_INIT_CRC16;
	int type;
	int n;

	/* First, calculate packet header byte */
	if ((type = HEADER_TYPE[ls_txCRC32][ls_UseVarHeaders][ls_txSendRLE]) < 0) {
		log("zmodem: invalid options set, %s, %s, %s",
			ls_txCRC32?"crc32":"crc16",
			ls_UseVarHeaders?"varh":"fixh",
			ls_txSendRLE?"RLE":"Plain");
		return ERROR;
	}

	/* Send *<DLE> and packet type */
	BUFCHAR(ZPAD);BUFCHAR(ZDLE);BUFCHAR(type);
	if (ls_UseVarHeaders) ls_sendchar(len);			/* Send length of header, if needed */

	ls_sendchar(frametype);							/* Send type of frame */
	crc = LSZ_UPDATE_CRC(frametype,crc);
	/* Send whole header */
	for (n=len; --n >= 0; ++hdr) {
		ls_sendhex(*hdr);
		crc = LSZ_UPDATE_CRC16((0xFF & *hdr),crc);
	}
	if (ls_txCRC32) {
		crc = ~crc;
		crc = LTOI(crc);
		for (n=4; --n >= 0;) {
			ls_sendchar(crc & 0xFF);
			crc >>= 8;
		}
	} else {
		crc = LSZ_UPDATE_CRC(0,LSZ_UPDATE_CRC(0,crc));
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
	int crc = LSZ_INIT_CRC16;
	int n;

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
	crc = LSZ_UPDATE_CRC16(frametype,crc);
	/* Send whole header */
	for (n=len; --n >= 0; ++hdr) {
		ls_sendchar(*hdr);
		crc = LSZ_UPDATE_CRC16((0xFF & *hdr),crc);
	}
	crc = LSZ_UPDATE_CRC16(0,LSZ_UPDATE_CRC16(0,crc));
	crc &= 0xFFFF;
	crc = STOI(crc);
	ls_sendhex(crc >> 8);
	ls_sendhex(crc & 0xFF);
	/* Clean buffer, do real send */
	return BUFLUSH();
}

int ls_zrecvhdr(char *hdr, int timeout)
{
	static enum rhSTATE {
		rhInit,				/* Start state */
		rhZPAD,				/* ZPAD got (*) */
		rhZDLE,				/* We got ZDLE */
		rhZBIN,
		rhZHEX,
		rhZBIN32,
		rhZBINR32,
		rhZVBIN,
		rhZVHEX,
		rhZVBIN32,
		rhZVBINR32,
		rhBYTE,
		rhCRC
	} state = rhInit;
	static enum rhREADMODE {
		rm7BIT,
		rmZDLE,
		rmHEX
	} readmode = rm7BIT;

	static int frametype = LSZ_ERROR;
	static int crc = 0;
	static int crcl = 2;
	static int crcgot = 0;
	static int incrc = 0;
	static int len = 0;
	static int got = 0;
	static int inhex = 0;
	int t = time(NULL);
	int c;
	int res;

	if(rhInit == state) {
		crc = 0;
		crcl = 2;
		crcgot = 0;
		incrc = LSZ_INIT_CRC16;
		len = 0;
		got = 0;
		inhex = 0;
		readmode = rm7BIT;
	}

	while(OK == (res = HASDATA(timeout))) {
		switch(readmode) {
		case rm7BIT: c = ls_read7bit(0); break;
		case rmZDLE: c = ls_readzdle(0); break;
		case rmHEX: c = ls_readhex(0); break;
		}
		if(c < 0) return c;
		timeout -= (time(NULL) - t); t = time(NULL);

		switch(state) {
		case rhInit:
			if(ZPAD == c) { state = rhZPAD; }
			else { ls_Garbage++; }
			break;
		case rhZPAD:
			switch(c) {
			case ZPAD: break;
			case ZDLE: state = rhZDLE; break;
			default: ls_Garbage++; state = rhInit; break;
			}
			break;
		case rhZDLE:
			switch(c) {
			case ZBIN: state = rhZBIN; frametype = c; readmode = rmZDLE; break;
			case ZHEX: state = rhZHEX; frametype = c; readmode = rmHEX; break;
			case ZBIN32: state = rhZBIN32; frametype = c; readmode = rmZDLE; break;
			case ZBINR32: state = rhZBINR32; frametype = c; readmode = rmZDLE; break;
			case ZVBIN: state = rhZVBIN; frametype = c; readmode = rmZDLE; break;
			case ZVHEX: state = rhZVHEX; frametype = c; readmode = rmHEX; break;
			case ZVBIN32: state = rhZVBIN32; frametype = c; readmode = rmZDLE; break;
			case ZVBINR32: state = rhZVBINR32; frametype = c; readmode = rmZDLE; break;
			default: ls_Garbage++; state = rhInit; readmode = rm7BIT; break;
			}
			break;
		case rhZVBIN32:
		case rhZVBINR32:
			crcl = 4; /* Fall */
		case rhZVBIN:
		case rhZVHEX:
			len = c;
			state = rhBYTE;
			break;
		case rhZBIN32:
		case rhZBINR32:
			crcl = 4; /* Fall */
		case rhZBIN:
		case rhZHEX:
			len = 4;
			state = rhBYTE;
			break;
		case rhBYTE:
			hdr[got] = c;
			if(!(--len)) state = rhCRC;
			if(2 == crcl) { incrc = LSZ_UPDATE_CRC16(c,incrc); } 
			else { incrc = LSZ_UPDATE_CRC32(c,incrc); }
			break;
		case rhCRC:
			crc <<= 8;
			crc |= c & 0xFF;
			if(++crcgot == crcl)  { /* Crc finished */
				state = rhInit;
				if(2 == crcl) {
					crc = STOH(crc & 0xFFFF);
					incrc = LSZ_UPDATE_CRC16(0,LSZ_UPDATE_CRC16(0,incrc)) & 0xFFFF;
					if (incrc != crc) return LSZ_BADCRC;
					return frametype;
				} else {
					crc = LTOH(crc);
					if (incrc != crc) return LSZ_BADCRC;
					return frametype;
				}
			}
			break;
		default:
			break;
		}
	}
	return res;
}

/* Send one char with escaping */
void ls_sendchar(int c) 
{
	int esc = 0;
	c &= 0xFF;
	if (ls_Protocol & LSZ_DIRZAP) {	/* We are Direct ZedZap -- escape only <DLE> */
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
		c ^= 0x40;
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
	} while((0 == (ls_Protocol & LSZ_DIRZAP)) && (XON == c || XOFF == c));

	if (CAN == c) { if (++ls_CANCount == 5) return LSZ_CAN; }
	else { ls_CANCount = 0; }
	return c & 0x7F;
}

/* Read one hex character */
int ls_readhexnibble(int timeout) {
	int c;
	if((c = GETCHAR(timeout)) < 0) return c;
	if (CAN == c) { if (++ls_CANCount == 5) return LSZ_CAN; }
	else { ls_CANCount = 0; }
	if(c >= '0' && c <= '9') {
		return c - '0';
	} else if(c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	} else {
		return 0; /* will be CRC error */
	}
}

/* Read chracter as two hex digit */
int ls_readhex(int timeout)
{
	static int c = 0;
	int c2;
	int t = time(NULL);
	int res;

	if(!ls_GotHexNibble) {
		if((c = ls_readhexnibble(timeout)) < 0) return c;
		timeout -= (time(NULL) - t); t = time(NULL);
		c <<= 4;
	}
	if(OK == (res = HASDATS(timeout))) {
		if((c2 = ls_readhexnibble(0)) < 0) return c2;
        ls_GotHexNibble = 0;
		return c | c2;
	} else {
		ls_GotHexNibble = 1;
		return res;
	}
}

/* Retrun 8bit character, strip <DLE> */
int ls_readzdle(int timeout)
{
	int c;

	if(!ls_GotZDLE) { /* There was no ZDLE in stream, try to read one */
		do {
			if((c = GETCHAR(timeout)) < 0) return c;
			if (CAN == c) { if (++ls_CANCount == 5) return LSZ_CAN; }
			else { ls_CANCount = 0; }

			if(!(ls_Protocol & LSZ_DIRZAP)) { /* Check for unescaped XON/XOFF */
				switch(c) {
				case XON: case XON | 0x80:
				case XOFF: case XOFF | 0x80:
					c = LSZ_XONXOFF;
				}
			}
			if (ZDLE == c) { ls_GotZDLE = 1; }
			else if(LSZ_XONXOFF != c) { return c & 0xFF; }
		} whlie(LSZ_XONXOFF == c);
	}
	/* We will be here only in case of DLE */
	if(HASDATA(0)) { /* We have data RIGHT NOW! */
		ls_GotZDLE = 0;
		if((c = GETCHAR(0)) < 0) return c;
		if (CAN == c) { if (++ls_CANCount == 5) return LSZ_CAN; }
        ls_CANCount = 0;
        switch(c) {
		case ZCRCE: return LSZ_CRCE;
		case ZCRCG: return LSZ_CRCG;
		case ZCRCQ: return LSZ_CRCQ;
		case ZCRCW: return LSZ_CRCW;
		case ZRUB0: return ZDEL;
		case ZRUB1: return ZDEL | 0x80;
		default:
			return (c ^ 0x40) & 0xFF;
        }
	}
}
