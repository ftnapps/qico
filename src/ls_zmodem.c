/*
   ZModem file transfer protocol. Written from scratches.
   Support CRC16, CRC32, variable header, ZedZap (big blocks) and DirZap.
   Global variables, common functions.
   $Id: ls_zmodem.c,v 1.1.1.1 2003/07/12 21:26:50 sisoft Exp $
*/
#include "headers.h"
#include "defs.h"
#include "ls_zmodem.h"

/* Common variables */
byte ls_txHdr[LSZ_MAXHLEN];	/* Sended header */
byte ls_rxHdr[LSZ_MAXHLEN];	/* Receiver header */
int ls_GotZDLE;				/* We seen DLE as last character */
int ls_GotHexNibble;		/* We seen one hex digit as last character */
int ls_Protocol;			/* Plain/ZedZap/DirZap and other options */
int ls_CANCount;			/* Count of CANs to go */
int ls_Garbage;				/* Count of garbage characters */
int ls_SerialNum;			/* Serial number of file -- for Double-skip protection */
int ls_HeaderTimeout;		/* Timeout for headers */
int ls_DataTimeout;			/* Timeout for data blocks */
int ls_MaxBlockSize;		/* Maximum block size */
int ls_SkipGuard;			/* double-skip protection on/off */

/* Variables to control sender */
int ls_txWinSize;			/* Receiver Window/Buffer size (0 for streaming) */
int ls_rxCould;				/* Receiver could fullduplex/streamed IO (from ZRINIT) */
int ls_txCurBlockSize;		/* Current block size */
int ls_txLastSent;			/* Last sent character -- for escaping */
long ls_txLastACK;			/* Last ACKed byte */
long ls_txLastRepos;		/* Last requested byte */
long ls_txReposCount;		/* Count of REPOSes on one position */
long ls_txGoodBlocks;		/* Good blocks sent */

/* Variables to control receiver */


/* String names of frames, if debug only */
#ifdef NEED_DEBUG
char *LSZ_FRAMETYPES[] = {
	"XONXOFF",			/* -8 */
	"BADCRC",			/* -7 */
	"NOHEADER",			/* -6 */
	"ERROR",			/* -5 */
	"CAN",				/* -4 */
	"RCDO",				/* -3 */
	"TIMEOUT",			/* -2 */
	"STRERR",			/* -1? */
	"ZRQINIT",
	"ZRINIT",
	"ZSINIT",
	"ZACK",
	"ZFILE",
	"ZSKIP",
	"ZNAK",
	"ZABORT",
	"ZFIN",
	"ZRPOS",
	"ZDATA",
	"ZEOF",
	"ZFERR",
	"ZCRC",
	"ZCHALLENGE",
	"ZCOMPL",
	"ZCAN",
	"ZFREECNT",
	"ZCOMMAND",
	"ZSTDERR"
};
#endif


/* Special table to FAST calculate header type */
/*                 CRC32,VAR,RLE */
static int HEADER_TYPE[2][2][2] = {{{ZBIN,-1},{ZVBIN,-1}},{{ZBIN32,ZBINR32},{ZVBIN32,ZVBINR32}}};

/* Hex digits */
static char HEX_DIGITS[] = "0123456789abcdef";


unsigned char inchatbuf[16384],*chattxbuf;
unsigned short inchatfill=0;
static int chattxstate=CHAT_DONE;

/* Functions */

/* Send binary header. Use proper CRC, send var. len. if could */
int ls_zsendbhdr(int frametype, int len, byte *hdr)
{
	long crc = LSZ_INIT_CRC;
	int type;
	int n;

	/* First, calculate packet header byte */
	if ((type = HEADER_TYPE[(ls_Protocol & LSZ_OPTCRC32)==LSZ_OPTCRC32][(ls_Protocol & LSZ_OPTVHDR)==LSZ_OPTVHDR][(ls_Protocol & LSZ_OPTRLE)==LSZ_OPTRLE]) < 0) {
		write_log("zmodem link options error: %s, %s, %s",
				(ls_Protocol & LSZ_OPTCRC32)?"CRC32":"CRC16",
				(ls_Protocol & LSZ_OPTVHDR)?"VHDR":"HDR",
				(ls_Protocol & LSZ_OPTRLE)?"RLE":"Plain");
		return LSZ_ERROR;
	}
	DEBUG(('Z',1,"ls_zsendbhdr: %c, %s, len: %d",type,LSZ_FRAMETYPES[frametype+LSZ_FTOFFSET],len));

	/* Send *<DLE> and packet type */
	BUFCHAR(ZPAD);BUFCHAR(ZDLE);BUFCHAR(type);
	if (ls_Protocol & LSZ_OPTVHDR) ls_sendchar(len);			/* Send length of header, if needed */
	else len = 4;

	ls_sendchar(frametype);							/* Send type of frame */
	crc = LSZ_UPDATE_CRC(frametype,crc);
	/* Send whole header */
	for (n=0; n < len; n++) {
		ls_sendchar(*hdr);
		crc = LSZ_UPDATE_CRC((unsigned char)(*hdr),crc);
    		hdr++;
	}
	crc = LSZ_FINISH_CRC(crc);
	DEBUG(('Z',2,"ls_zsendbhdr: CRC%d is %08x",(ls_Protocol&LSZ_OPTCRC32)?32:16,crc));
	if (ls_Protocol & LSZ_OPTCRC32) {
		crc = LTOI(crc);
		for (n=0;n<4;n++) { ls_sendchar(crc & 0xff); crc >>= 8; }
	} else {
		crc = STOI(crc & 0xffff);
		ls_sendchar(crc >> 8);
		ls_sendchar(crc & 0xff);
	}
	/* Clean buffer, do real send */
	return BUFFLUSH();
}

/* Send HEX header. Use CRC16, send var. len. if could */
int ls_zsendhhdr(int frametype, int len, byte *hdr)
{
	long crc = LSZ_INIT_CRC16;
	int n;

	DEBUG(('Z',1,"ls_zsendhhdr: %s, len: %d",LSZ_FRAMETYPES[frametype+LSZ_FTOFFSET],len));
	/* Send **<DLE> */
	BUFCHAR(ZPAD); BUFCHAR(ZPAD); BUFCHAR(ZDLE);
	/* Send header type */
	if (ls_Protocol & LSZ_OPTVHDR) {
		BUFCHAR(ZVHEX);
		ls_sendhex(len);
	} else {
		BUFCHAR(ZHEX);
		len = 4;
	}

	ls_sendhex(frametype);
	crc = LSZ_UPDATE_CRC16(frametype,crc);
	/* Send whole header */
	for (n=0; n < len; n++) {
		ls_sendhex(*hdr);
		crc = LSZ_UPDATE_CRC16((0xff & *hdr),crc);
		hdr++;
	}
	crc = LSZ_FINISH_CRC16(crc);
	DEBUG(('Z',2,"ls_zsendhhdr: CRC16 is %04x",crc));
	crc = STOI(crc & 0xffff);
	ls_sendhex(crc >> 8);
	ls_sendhex(crc & 0xff);
	BUFCHAR(CR);
	BUFCHAR(LF|(char)0x80);
	if(frametype != ZACK && frametype != ZFIN) BUFCHAR(XON);
	/* Clean buffer, do real send */
	return BUFFLUSH();
}

int ls_zrecvhdr(byte *hdr, int *hlen, int timeout)
{
	static enum rhSTATE {
		rhInit,				/* Start state */
		rhZPAD,				/* ZPAD got (*) */
		rhZDLE,				/* We got ZDLE */
		rhFrameType,
		rhZBIN,
		rhZHEX,
		rhZBIN32,
		rhZBINR32,
		rhZVBIN,
		rhZVHEX,
		rhZVBIN32,
		rhZVBINR32,
		rhBYTE,
		rhCRC,
		rhCR,
		rhLF
	} state = rhInit;
	static enum rhREADMODE {
		rm8BIT,
		rm7BIT,
		rmZDLE,
		rmHEX
	} readmode = rm7BIT;

	static int frametype = LSZ_ERROR;	/* Frame type */
	static int crcl = 2;				/* Length of CRC (CRC16 is default) */
	static int crcgot = 0;				/* Number of CRC bytes already got */
	static long incrc = 0;				/* Calculated CRC */
	static long crc = 0;				/* Received CRC */
	static int len = 4;					/* Length of header (4 is default) */
	static int got = 0;					/* Number of header bytes already got */
	static int inhex = 0;
	int c = -1;
	int rc;

	DEBUG(('Z',1,"ls_zrecvhdr: timeout %d",timeout));

	if(rhInit == state) {
		DEBUG(('Z',2,"ls_zrecvhdr: init state"));
		frametype = LSZ_ERROR;
		crc = 0;
		crcl = 2;
		crcgot = 0;
		incrc = LSZ_INIT_CRC16;
		len = 4;
		got = 0;
		inhex = 0;
		readmode = rm7BIT;
	}

	while(OK == (rc = HASDATAT(&timeout))) {
		switch(readmode) {
		case rm8BIT: c = ls_readcanned(&timeout); break;
		case rm7BIT: c = ls_read7bit(&timeout);   break;
		case rmZDLE: c = ls_readzdle(&timeout);   break;
		case rmHEX:  c = ls_readhex(&timeout);    break;
		}
		if(c < 0) return c;								/* Here is error */
		c &= 0xff;										/* Strip high bits */

		switch(state) {
		case rhInit:
			if(ZPAD == c) { state = rhZPAD; }
			else { ls_Garbage++; }
			break;
		case rhZPAD:
			DEBUG(('Z',2,"ls_zrecvhdr: rhZPAD, garbage counter: %d",ls_Garbage));
			switch(c) {
			case ZPAD: break;
			case ZDLE: state = rhZDLE; break;
			default: ls_Garbage++; state = rhInit; break;
			}
			break;
		case rhZDLE:
			DEBUG(('Z',2,"ls_zrecvhdr: rhZDLE, got: %02x (%c)",(unsigned char)c,(unsigned char)c));
			switch(c) {
			case ZBIN: state = rhZBIN; readmode = rmZDLE; break;
			case ZHEX: state = rhZHEX; readmode = rmHEX; break;
			case ZBIN32: state = rhZBIN32; readmode = rmZDLE; break;
			case ZVBIN: state = rhZVBIN; readmode = rmZDLE; break;
			case ZVHEX: state = rhZVHEX; readmode = rmHEX; break;
			case ZVBIN32: state = rhZVBIN32; readmode = rmZDLE; break;
			default: ls_Garbage++; state = rhInit; readmode = rm7BIT; break;
			}
			break;
		case rhZVBIN32:
			crcl = 4;
			/* Fall throught */
		case rhZVBIN:
		case rhZVHEX:
			if(c > LSZ_MAXHLEN) {
				DEBUG(('Z',1,"ls_zrecvhdr: Header TOO long: %d bytes (state: %d, CRC%d)",c,(int)state,(2==crcl?16:32)));
				state = rhInit;
				return LSZ_BADCRC;
			}
			DEBUG(('Z',2,"ls_zrecvhdr: Any rhZV: %d bytes (state: %d, CRC%d)",c,(int)state,(2==crcl?16:32)));
			len = c;
			state = rhFrameType;
			break;
		case rhZBIN32:
			crcl = 4;
			/* Fall throught */
		case rhZBIN:
		case rhZHEX:
			if(c < 0 || c > LSZ_MAXFRAME) {
				DEBUG(('Z',1,"ls_zrecvhdr: Unknown frame type: %d (state: %d, CRC%d)",c,(int)state,(2==crcl?16:32)));
				state = rhInit;
				return LSZ_BADCRC;
			}
			DEBUG(('Z',1,"ls_zrecvhdr: Any rhZ frametype: %d, %s (state: %d, CRC%d)",c,LSZ_FRAMETYPES[c+LSZ_FTOFFSET],(int)state,(2==crcl?16:32)));
			len = 4;
			frametype = c;
			if(2 == crcl) { incrc = LSZ_UPDATE_CRC16((unsigned char)c,LSZ_INIT_CRC16); }
			else { incrc = LSZ_UPDATE_CRC32((unsigned char)c,LSZ_INIT_CRC32); }
			state = rhBYTE;
			break;
		case rhFrameType:
			if(c < 0 || c > LSZ_MAXFRAME) {
				DEBUG(('Z',1,"ls_zrecvhdr: Unknown frame type: %d (state: %d, CRC%d)",c,(int)state,(2==crcl?16:32)));
				state = rhInit;
				return LSZ_BADCRC;
			}
			DEBUG(('Z',1,"ls_zrecvhdr: frametype %d, %s",c,LSZ_FRAMETYPES[c+LSZ_FTOFFSET]));
			frametype = c;
			if(2 == crcl) { incrc = LSZ_UPDATE_CRC16((unsigned char)c,LSZ_INIT_CRC16); }
			else { incrc = LSZ_UPDATE_CRC32((unsigned char)c,LSZ_INIT_CRC32); }
			state = rhBYTE;
			break;			
		case rhBYTE:
			DEBUG(('Z',2,"ls_zrecvhdr: rhBYTE: %02x",c));
			hdr[got] = c;
			if(++got == len) state = rhCRC;
			if(2 == crcl) { incrc = LSZ_UPDATE_CRC16((unsigned char)c,incrc); } 
			else { incrc = LSZ_UPDATE_CRC32((unsigned char)c,incrc); }
			break;
		case rhCRC:
			DEBUG(('Z',2,"ls_zrecvhdr: rhCRC"));
			if(2 == crcl) { crc <<= 8; crc |= (unsigned char)c; }
			else { crc |= (unsigned long)c << (8*crcgot); }
			if(++crcgot == crcl)  { /* Crc finished */
				state = rhInit;
				ls_Garbage = 0;
				if(2 == crcl) {
#ifdef NEED_DEBUG
					if(ls_Protocol&LSZ_OPTCRC32 && rmHEX!=readmode) DEBUG(('Z',1,"ls_zrecvhdr: was CRC32, got CRC16 binary header"));
#endif
					incrc = LSZ_FINISH_CRC16(incrc); crc = STOH(crc & 0xffff);
					if(rmHEX!=readmode) ls_Protocol &= (~LSZ_OPTCRC32);
				} else {
#ifdef NEED_DEBUG
					if(!(ls_Protocol&LSZ_OPTCRC32)) DEBUG(('Z',1,"ls_zrecvhdr: was CRC16, got CRC32 binary header"));
#endif
					incrc = LSZ_FINISH_CRC32(incrc); crc = LTOH(crc);
					ls_Protocol |= LSZ_OPTCRC32;
				}
				DEBUG(('Z',2,"ls_zrecvhdr: CRC%d got %08x, claculated %08x",(2==crcl)?16:32,incrc,crc));
				if (incrc != crc) return LSZ_BADCRC;
				*hlen = got;
				/* We need to read <CR><LF> after HEX header */
				if(rmHEX == readmode) { state = rhCR; readmode = rm8BIT; }
				else { return frametype; }
			}
			break;
		case rhCR:
			state = rhInit;
			DEBUG(('Z',2,"ls_zrecvhdr: rhCR"));
			switch(c) {
			case CR:
			case CR|0x80:		/* we need LF after <CR> */
				state = rhLF;
				break;
			case LF:
			case LF|0x80:		/* Ok, UNIX-like EOL */
				return frametype;
			default:
				return LSZ_BADCRC;
			}
			break;
		case rhLF:
			state = rhInit;
			DEBUG(('Z',2,"ls_zrecvhdr: rhLF"));
			switch(c) {
			case LF:
			case LF|0x80:
				return frametype;
			default:
				return LSZ_BADCRC;
			}
			break;
		default:
			break;
		}
	}
	DEBUG(('Z',1,"ls_zrecvhdr: timeout ot something other: %d, %s",rc,LSZ_FRAMETYPES[rc+LSZ_FTOFFSET]));
	return rc;
}

/* Send data block, with CRC16 and framing */
int ls_zsenddata(byte *data, int len, int frame)
{
	long crc;

	DEBUG(('Z',1,"ls_zsenddata: %d bytes, %c frameend",len,(char)frame));

	if (ls_Protocol & LSZ_OPTCRC32) {
		crc = LSZ_INIT_CRC32;
		for(;len--; data++) {
			ls_sendchar(*data);
			crc = LSZ_UPDATE_CRC32((unsigned char)(*data),crc);
		}
		BUFCHAR(ZDLE); BUFCHAR(frame);
		crc = LSZ_UPDATE_CRC32(frame,crc);
//chat
		if(rnode->chat) {
			if(frame==ZCRCG||frame==ZCRCW)z_devsend_c(1);
			BUFCHAR(0);
		}
//
		crc = LSZ_FINISH_CRC32(crc);
		crc = LTOI(crc);
		DEBUG(('Z',2,"ls_zsenddata: CRC32 is %08x",crc));
		ls_sendchar(crc&0xff); crc >>= 8;
		ls_sendchar(crc&0xff); crc >>= 8;
		ls_sendchar(crc&0xff); crc >>= 8;
		ls_sendchar(crc&0xff); crc >>= 8;
	} else {
		crc = LSZ_INIT_CRC16;
		for(;len--; data++) {
			ls_sendchar(*data);
			crc = LSZ_UPDATE_CRC16((unsigned char)(*data),crc);
		}
		BUFCHAR(ZDLE); BUFCHAR(frame);
		crc = LSZ_UPDATE_CRC16(frame,crc);
//chat
		if(rnode->chat) {
			if(frame==ZCRCG||frame==ZCRCW)z_devsend_c(1);
			BUFCHAR(0);
		}
//
		crc = LSZ_FINISH_CRC16(crc);
		crc = STOI(crc & 0xffff);
		DEBUG(('Z',2,"ls_zsenddata: CRC16 is %04x",crc));
		ls_sendchar(crc >> 8);
		ls_sendchar(crc & 0xff);
	}

	if(!(ls_Protocol & LSZ_OPTDIRZAP) && ZCRCW == frame) BUFCHAR(XON);
	return BUFFLUSH();
}

/* Receive data subframe with CRC16, return frame type or error (may be -- timeout) */
int ls_zrecvdata16(byte *data, int *len, int timeout)
{
	int c;
	int got = 0;					/* Bytes total got */
	long incrc = LSZ_INIT_CRC16;	/* Calculated CRC */
	long crc = 0;					/* Received CRC */
	int frametype = LSZ_ERROR;		/* Type of frame - ZCRC(G|W|Q|E) */
	int rcvdata = 1;				/* Data is being received NOW (not CRC) */

	DEBUG(('Z',1,"ls_zrecvdata16: timeout %d",timeout));

	while(rcvdata && ((c = ls_readzdle(&timeout)) >= 0)) {
		if(c < 256) {
			*data++ = c & 0xff;
			if(++got > ls_MaxBlockSize) {
				DEBUG(('Z',1,"ls_zrecvdata16: Block is too big (%d/%d, %02x and %02x)",got,ls_MaxBlockSize,*(data-1),c));
				return LSZ_BADCRC;
			}
			incrc = LSZ_UPDATE_CRC16((unsigned char)c,incrc);
		} else {
			switch(c) {
			case LSZ_CRCE:
			case LSZ_CRCG:
			case LSZ_CRCQ:
			case LSZ_CRCW:
				rcvdata = 0;
				frametype = c & 0xff;
				DEBUG(('Z',2,"ls_zrecvdata16: frameend %c",(char)frametype));
				incrc = LSZ_UPDATE_CRC16((unsigned char)c,incrc);
				break;
			default:
				DEBUG(('Z',1,"ls_zrecvdata16: bad frameend %d",c));
				return LSZ_BADCRC;
			}
		}
	}
	/* We finish loop by error in ls_readzdle() */
	if(rcvdata) {
		DEBUG(('Z',1,"ls_zrecvdata16: timeout or something else: %d, %s",c,LSZ_FRAMETYPES[c+LSZ_FTOFFSET]));
		return c;
	}

	/* Loops ar unrolled */
	if((c = ls_readzdle(&timeout)) < 0) return c;
	crc = (unsigned char)c;
	if((c = ls_readzdle(&timeout)) < 0) return c;
	crc <<= 8; crc |= (unsigned char)c;
	crc = STOH(crc);
	incrc = LSZ_FINISH_CRC16(incrc);
	DEBUG(('Z',2,"ls_zrecvdata16: CRC16 got %04x, claculated %04x",incrc,crc));

	if (incrc != crc) return LSZ_BADCRC;
	*len = got;
	DEBUG(('Z',1,"ls_zrecvdata16: OK"));
	return frametype;
}

/* Receive data subframe with CRC32, return frame type or error (may be -- timeout) */
int ls_zrecvdata32(byte *data, int *len, int timeout)
{
	int c;
	int got = 0;					/* Bytes total got */
	long incrc = LSZ_INIT_CRC32;	/* Calculated CRC */
	long crc = 0;					/* Received CRC */
	int frametype = LSZ_ERROR;		/* Type of frame - ZCRC(G|W|Q|E) */
	int rcvdata = 1;				/* Data is being received NOW (not CRC) */

	DEBUG(('Z',1,"ls_zrecvdata32: timeout %d",timeout));

	while(rcvdata && ((c = ls_readzdle(&timeout)) >= 0)) {
		if(c < 256) {
			*data++ = c & 0xff;
			if(++got > ls_MaxBlockSize) {
				DEBUG(('Z',1,"ls_zrecvdata32: Block is too big (%d/%d, %02x and %02x)",got,ls_MaxBlockSize,*(data-1),c));
				return LSZ_BADCRC;
			}
			incrc = LSZ_UPDATE_CRC32((unsigned char)c,incrc);
		} else {
			switch(c) {
			case LSZ_CRCE:
			case LSZ_CRCG:
			case LSZ_CRCQ:
			case LSZ_CRCW:
				rcvdata = 0;
				frametype = c & 0xff;
				DEBUG(('Z',2,"ls_zrecvdata21: frameend %c",(char)frametype));
				incrc = LSZ_UPDATE_CRC32((unsigned char)c,incrc);
				break;
			default:
				DEBUG(('Z',1,"ls_zrecvdata32: bad frameend %d",c));
				return LSZ_BADCRC;
			}
		}
	}
	/* We finish loop by error in ls_readzdle() */
	if(rcvdata) {
		DEBUG(('Z',1,"ls_zrecvdata32: timeout or something else: %d, %s",c,LSZ_FRAMETYPES[c+LSZ_FTOFFSET]));
		return c;
	}

	/* Loops ar unrolled */
	if((c = ls_readzdle(&timeout)) < 0) return c;
	crc |= (unsigned long)c << 0x00;
	if((c = ls_readzdle(&timeout)) < 0) return c;
	crc |= (unsigned long)c << 0x08;
	if((c = ls_readzdle(&timeout)) < 0) return c;
	crc |= (unsigned long)c << 0x10;
	if((c = ls_readzdle(&timeout)) < 0) return c;
	crc |= (unsigned long)c << 0x18;
	crc = LTOH(crc);
	incrc = LSZ_FINISH_CRC32(incrc);
	DEBUG(('Z',2,"ls_zrecvdata32: CRC32 got %08x, claculated %08x",incrc,crc));

	if (incrc != crc) return LSZ_BADCRC;
	*len = got;
	DEBUG(('Z',2,"ls_zrecvdata32: OK"));
	return frametype;
}

/* Send one char with escaping */
void ls_sendchar(int c) 
{
	int esc = 0;
	c &= 0xff;
	if (ls_Protocol & LSZ_OPTDIRZAP) {	/* We are Direct ZedZap -- escape only <DLE> */
		esc = (ZDLE == c);
	} else {			/* We are normal ZModem (may be ZedZap) */
		if ((ls_Protocol & LSZ_OPTESCAPEALL) && ((c & 0x60) == 0)) { /* Receiver want to escape ALL */
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
				esc = (((ls_txLastSent & 0x7f) == (char)'@') && ((c & 0x7f) == CR));
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
	char c = (char)(i & 0xff);
	BUFCHAR(HEX_DIGITS[(c & 0xf0) >> 4]);
	BUFCHAR(ls_txLastSent = HEX_DIGITS[c & 0x0f]);
}

/* Retrun 7bit character, strip XON/XOFF if not DirZap, with timeout */
int ls_read7bit(int *timeout)
{
	int c;

	do {
		if((c = GETCHART(timeout)) < 0) return c;
	} while((0 == (ls_Protocol & LSZ_OPTDIRZAP)) && (XON == c || XOFF == c));

	if (CAN == c) { if (++ls_CANCount == 5) return LSZ_CAN; }
	else { ls_CANCount = 0; }
	return c & 0x7f;
}

/* Read one hex character */
int ls_readhexnibble(int *timeout) {
	int c;
	if((c = ls_readcanned(timeout)) < 0) return c;
	if(c >= '0' && c <= '9') {
		return c - '0';
	} else if(c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	} else {
		return 0; /* will be CRC error */
	}
}

/* Read chracter as two hex digit */
int ls_readhex(int *timeout)
{
	static int c = 0;
	int c2;

	if(!ls_GotHexNibble) {
		if((c = ls_readhexnibble(timeout)) < 0) return c;
		c <<= 4;
	}
	if((c2 = ls_readhexnibble(timeout)) >= 0) {
        ls_GotHexNibble = 0;
		return c | c2;
	} else {
		ls_GotHexNibble = 1;
		return c2;
	}
}

/* Retrun 8bit character, strip <DLE> */
int ls_readzdle(int *timeout)
{
	int c,r=0,rr;

	if(!ls_GotZDLE) { /* There was no ZDLE in stream, try to read one */
		do {
			if((c = ls_readcanned(timeout)) < 0) return c;

			if(!(ls_Protocol & LSZ_OPTDIRZAP)) { /* Check for unescaped XON/XOFF */
				switch(c) {
				case XON: case XON | 0x80:
				case XOFF: case XOFF | 0x80:
					c = LSZ_XONXOFF;
				}
			}
			if (ZDLE == c) {
				ls_GotZDLE = 1;
			} else if(LSZ_XONXOFF != c) { return c & 0xff; }
		} while(LSZ_XONXOFF == c);
	}
	/* We will be here only in case of DLE */
	if((c = ls_readcanned(timeout)) >= 0) { /* We have data */
		ls_GotZDLE = 0;
        switch(c) {
		case ZCRCE:
			r=LSZ_CRCE;
			break;
		case ZCRCG:
			r=LSZ_CRCG;
			break;
		case ZCRCW:
			r=LSZ_CRCW;
			break;
		case ZCRCQ:
			r=LSZ_CRCQ;
			break;
	}																	
//chat
	if(rnode->chat&&r) {
		do {
			rr=ls_readcanned(timeout);
			z_devrecv_c(rr,0);
		} while(rr);
		z_devsend_c(0);
	}
//
	if(r)return r;
	switch(c) {
		case ZRUB0:
			return ZDEL;
		case ZRUB1:
			return ZDEL | 0x80;
		default:
			if((c&0x60) != 0x40) {
				DEBUG(('Z',1,"ls_readzdle: bad ZDLed character %02x (%02x)",c,(c ^ 0x40) & 0xff));
				return LSZ_BADCRC;
			}
			return (c ^ 0x40) & 0xff;
        }
	}
	return c;
}

/* Read one character, check for five CANs */
int ls_readcanned(int *timeout)
{
	int c;
	if ((c = GETCHART(timeout)) < 0) return c;
	if (CAN == c) { if (++ls_CANCount == 5) return LSZ_CAN; }
	else { ls_CANCount = 0; }
	return c & 0xff;
}

/* Store long integer (4 bytes) in buffer, as it must be stored in header */
void ls_storelong(byte *buf, long l)
{
	l=LTOI(l);
	buf[LSZ_P0] = (l)&0xff;
	buf[LSZ_P1] = (l>>8)&0xff;
	buf[LSZ_P2] = (l>>16)&0xff;
	buf[LSZ_P3] = (l>>24)&0xff;
}

/* Fetch long integer (4 bytes) from buffer, as it must be stored in header */
long ls_fetchlong(unsigned char *buf)
{
	long l = buf[LSZ_P3];
	l<<=8; l|= buf[LSZ_P2];
	l<<=8; l|= buf[LSZ_P1];
	l<<=8; l|= buf[LSZ_P0];
	return LTOH(l);
}

/* Send 8*CAN */
void ls_zabort()
{
	int i;
	BUFFLUSH();
	BUFCHAR(XON);
	for(i = 0; i < 8; i++) BUFCHAR(CAN);
	BUFFLUSH();
}

// chat routines
int z_devfree()
{
	if(chattxstate||!rnode->chat)return 0;
	    else return 1;
}

int z_devsend(unsigned char *data,unsigned short len)
{
	if(!data||!len||!z_devfree())return 0;
	chattxbuf=data;
	chattxbuf[(len>16383)?16383:len]=0;
	chattxstate=CHAT_DATA;
	return 1;
}

void z_devsend_c(int buffr)
{
	unsigned char *p;
	if(chattxstate==CHAT_DATA) {
		p=chattxbuf;
//		write_log("ss: %s",p);
		strtr((char*)p,ZPAD,(unsigned char)248);
		while(*p++)if(buffr)BUFCHAR(*p);
		    else ls_sendchar(*p);
		chattxstate=CHAT_DONE;
	}
}

void z_devrecv_c(unsigned char c,int flushed)
{
//	write_log("rr: %c",c);
	inchatbuf[inchatfill++]=c;
	if(c) {
		if(inchatfill>255||flushed) {
			inchatbuf[inchatfill]=0;
			goto flush;
		}
	    } else {
flush:		if(*inchatbuf) {
			strtr((char*)inchatbuf,(unsigned char)248,ZPAD);
			c_devrecv(inchatbuf,inchatfill);
		}
	inchatfill=0;
	}
}
