/**********************************************************
 * File: zm.c
 * Created at Fri Jul 16 17:22:13 1999 by pk // aaz@ruxy.org.ru
 * ZMODEM protocol primitives
 * $Id: zm.c,v 1.4 2000/10/07 13:48:28 lev Exp $
 **********************************************************/
/*
 *    Copyright 1994 Omen Technology Inc All Rights Reserved
 *
 * Entry point Functions:
 *	zsbhdr(type, hdr) send binary header
 *	zshhdr(type, hdr) send hex header
 *	zgethdr(hdr) receive header - binary or hex
 *	zsdata(buf, len, frameend) send data
 *	zrdata(buf, len) receive data
 *	stohdr(pos) store position data in Txhdr
 *	long rclhdr(hdr) recover position offset from header
 * 
 *
 *	This version implements numerous enhancements including ZMODEM
 *	Run Length Encoding and variable length headers.  These
 *	features were not funded by the original Telenet development
 *	contract.
 * 
 */
#include "mailer.h"
#include "defs.h"
#include "zmodem.h"
#include "qipc.h"

#ifdef ZZ_DEBUG
#undef PUTCHAR
#undef PUTSTR
#define PUTCHAR(x) deputch(x)

int deputch(int x)
{
	log("putchar %02x '%c'", x&0xff, (x>=32)?x:'.');
	return m_putc(x);
}
#endif

char *zbuffer=NULL;
int   zlength=0;

#define Rxtimeout 10		/* Tenths of seconds to wait for something */

/* Globals used by ZMODEM functions */
int Zctlesc;            /* Encode control characters */
int Zrwindow = 1400;    /* RX window size (controls garbage count) */

int Rxframeind;		/* ZBIN ZBIN32, or ZHEX type of frame */
int Rxtype;		/* Type of header received */
int Rxhlen;		/* Length of header received */
int Rxcount;		/* Count of data bytes received */
char Rxhdr[ZMAXHLEN];	/* Received header */
char Txhdr[ZMAXHLEN];	/* Transmitted header */
int Txfcs32;		/* TURE means send binary frames with 32 bit FCS */
int Crc32t;		/* Controls 32 bit CRC being sent */
/* 1 == CRC32,  2 == CRC32 + RLE */
int Crc32r;		/* Indicates/controls 32 bit CRC being received */
/* 0 == CRC16,  1 == CRC32,  2 == CRC32 + RLE */
int Usevhdrs;		/* Use variable length headers */
int Znulls;		/* Number of nulls to send at beginning of ZDATA hdr */
char Attn[ZATTNLEN+1];	/* Attention string rx sends to tx on err */
char *Altcan;		/* Alternate canit string */

static int lastsent;	/* Last char we sent */

static char *frametypes[] = {
	"ERROR",	/* -5 */
	"NORESP",	/* -4 */
	"RCD0",		/* -3 */
	"TIMEOUT",		/* -2 */
	"?ERROR?",		/* -1 */
#define FTOFFSET 5
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
	"ZSTDERR",
	"xxxxx"
#define FRTYPES 22	/* Total number of frame types in this array */
	/*  not including psuedo negative entries */
};

char badcrc[] = "Bad CRC";

/* Send ZMODEM binary header hdr of type type */
void zsbhdr(int len, int type, char *hdr)
{
	int n;
	unsigned short crc;

#ifdef Z_DEBUG
	log("zsbhdr: %s", frametypes[type+FTOFFSET]);
#endif
	BUFCLEAR();
	if (type == ZDATA)
		for (n = Znulls; --n >=0; )
			BUFCHAR(0);

	BUFCHAR(ZPAD); BUFCHAR(ZDLE);

	switch (Crc32t=Txfcs32) {
	case 2:
		zsbh32(len, hdr, type, Usevhdrs?ZVBINR32:ZBINR32);
/* 		FLUSH(); */
		break;
	case 1:
		zsbh32(len, hdr, type, Usevhdrs?ZVBIN32:ZBIN32);  break;
	default:
		if (Usevhdrs) {
			BUFCHAR(ZVBIN);
			zsendline(len);
		}
		else
			BUFCHAR(ZBIN);
		zsendline(type);
		crc = updcrc(type, 0);

		for (n=len; --n >= 0; ++hdr) {
			zsendline(*hdr);
			crc = updcrc((0377& *hdr), crc);
		}
		crc = updcrc(0,updcrc(0,crc));
		zsendline(crc>>8);
		zsendline(crc);
	}

	BUFFLUSH();
/* 	if (type != ZDATA) */
/* 		FLUSH(); */
}


/* Send ZMODEM binary header hdr of type type */
void zsbh32(int len, char *hdr, int type, int flavour)
{
	int n;
	unsigned long crc;

	BUFCHAR(flavour); 
	if (Usevhdrs) 
		zsendline(len);
	zsendline(type);
	crc = 0xFFFFFFFFL; crc = UPDC32(type, crc);

	for (n=len; --n >= 0; ++hdr) {
		crc = UPDC32((0377 & *hdr), crc);
		zsendline(*hdr);
	}
	crc = ~crc;
	for (n=4; --n >= 0;) {
		zsendline((int)crc);
		crc >>= 8;
	}
}

/* Send ZMODEM HEX header hdr of type type */
void zshhdr(int len, int type, char *hdr)
{
	int n;
	unsigned short crc;

#ifdef Z_DEBUG
	log("zsbhdr: %s", frametypes[type+FTOFFSET]);
#endif
	BUFCLEAR();
	BUFCHAR(ZPAD&0x7f); BUFCHAR(ZPAD&0x7f); BUFCHAR(ZDLE&0x7f);
	if (Usevhdrs) {
		BUFCHAR(ZVHEX&0x7f);
		zputhex(len);
	}
	else
		BUFCHAR(ZHEX&0x7f);
	zputhex(type);
	Crc32t = 0;

	crc = updcrc(type, 0);
	for (n=len; --n >= 0; ++hdr) {
		zputhex(*hdr); crc = updcrc((0377 & *hdr), crc);
	}
	crc = updcrc(0,updcrc(0,crc));
	zputhex(crc>>8); zputhex(crc);

	/* Make it printable on remote machine */
	BUFCHAR(015&0x7f);BUFCHAR(0212&0x7f);
	/*
	 * Uncork the remote in case a fake XOFF has stopped data flow
	 */
	if (type != ZFIN && type != ZACK)
		BUFCHAR(021&0x7f);

	BUFFLUSH();
}

/*
 * Send binary array buf of length length, with ending ZDLE sequence frameend
 */
#ifndef DSZ
char *Zendnames[] = { "ZCRCE", "ZCRCG", "ZCRCQ", "ZCRCW"};
#endif

void zsdata(char *buf, int length, int frameend)
{
	unsigned short crc;

	BUFCLEAR();
	switch (Crc32t) {
	case 1:
		zsda32(buf, length, frameend);  break;
	case 2:
		zsdar32(buf, length, frameend);  break;
	default:
		crc = 0;
		for (;--length >= 0; ++buf) {
			zsendline(*buf); crc = updcrc((0377 & *buf), crc);
		}
		BUFCHAR(ZDLE); BUFCHAR(frameend);
		crc = updcrc(frameend, crc);

		crc = updcrc(0,updcrc(0,crc));
		zsendline(crc>>8); zsendline(crc);
	}
	if (frameend == ZCRCW)
		BUFCHAR(XON);

	BUFFLUSH();
}

void zsda32(char *buf, int length, int frameend)
{
	int c;
	unsigned long crc;

	crc = 0xFFFFFFFFL;
	for (;--length >= 0; ++buf) {
		c = *buf & 0377;
		if (c & 0140)
			BUFCHAR(lastsent = c)
		else
			zsendline(c);
		crc = UPDC32(c, crc);
	}
	BUFCHAR(ZDLE); BUFCHAR(frameend);
	crc = UPDC32(frameend, crc);

	crc = ~crc;
	for (c=4; --c >= 0;) {
		zsendline((int)crc);  crc >>= 8;
	}
}

/*
 * Receive array buf of max length with ending ZDLE sequence
 *  and CRC.  Returns the ending character or error code.
 *  NB: On errors may store length+1 bytes!
 */
int zrdata(char *buf, int length)
{
	int c;
	unsigned short crc;
	char *end;
	int d;

	switch (Crc32r) {
	case 1:
		return zrdat32(buf, length);
	case 2:
		return zrdatr32(buf, length);
	}

	crc = Rxcount = 0;  end = buf + length;
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
		  crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				crc = updcrc((d=c)&0377, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc(c, crc);
				if (crc & 0xFFFF) {
					zperr(badcrc);
					return ERROR;
				}
				Rxcount = length - (end - buf);
				return d;
			case GOTCAN:
				zperr("Sender Canceled");
				return ZCAN;
			case TIMEOUT:
				zperr("TIMEOUT");
				return c;
			default:
				garbitch(); return c;
			}
		}
		*buf++ = c;
		crc = updcrc(c, crc);
	}
#ifdef DSZ
	garbitch(); 
#else
	zperr("Data subpacket too long");
#endif
	return ERROR;
}

int zrdat32(char *buf, int length)
{
	int c;
	unsigned long crc;
	char *end;
	int d;

	crc = 0xFFFFFFFFL;  Rxcount = 0;  end = buf + length;
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
		  crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				d = c;  c &= 0377;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if (crc != 0xDEBB20E3) {
					zperr(badcrc);
					return ERROR;
				}
				Rxcount = length - (end - buf);
				return d;
			case GOTCAN:
				zperr("Sender Canceled");
				return ZCAN;
			case TIMEOUT:
				zperr("TIMEOUT");
				return c;
			default:
				garbitch(); return c;
			}
		}
		*buf++ = c;
		crc = UPDC32(c, crc);
	}
	zperr("Data subpacket too long");
	return ERROR;
}

void garbitch()
{
	zperr("Garbled data subpacket");
}

/*
 * Read a ZMODEM header to hdr, either binary or hex.
 *
 *   Set Rxhlen to size of header (default 4) (valid iff good hdr)
 *  On success, set Zmodem to 1, set rxpos and return type of header.
 *   Otherwise return negative on error.
 *   Return ERROR instantly if ZCRCW sequence, for fast error recovery.
 */
int zgethdr(char *hdr)
{
	int c, n, cancount;

#ifdef Z_DEBUG	
	log("zgethdr waiting...");
#endif
	n = Zrwindow + effbaud;		/* Max bytes before start of frame */
	Rxframeind = Rxtype = 0;

  startover:
	cancount = 5;
  again:
	/* Return immediate ERROR if ZCRCW sequence seen */
	switch (c = GETCHAR(Rxtimeout)) {
	case 021: case 0221:
		goto again;
	case RCDO:
	case TIMEOUT:
	case ERROR:
		goto fifi;
	case CAN:
	  gotcan:
	  if (--cancount <= 0) {
		  c = ZCAN; goto fifi;
	  }
	  switch (c = GETCHAR(Rxtimeout)) {
	  case TIMEOUT:
		  goto again;
	  case ZCRCW:
		  switch (GETCHAR(Rxtimeout)) {
		  case TIMEOUT:
			  c = ERROR; goto fifi;
		  case RCDO:
			  goto fifi;
		  default:
			  goto agn2;
		  }
	  case RCDO:
	  case ERROR:
		  goto fifi;
	  default:
		  break;
	  case CAN:
		  if (--cancount <= 0) {
			  c = ZCAN; goto fifi;
		  }
		  goto again;
	  }
	  /* **** FALL THRU TO **** */
	default:
	  agn2:
	  if ( --n == 0) {
		  c = GCOUNT;  goto fifi;
	  }
	  goto startover;
	case ZPAD:		/* This is what we want. */
		break;
	}
	cancount = 5;
  splat:
	switch (c = noxrd7()) {
	case ZPAD:
		goto splat;
	case RCDO:
	case TIMEOUT:
	case ERROR:
		goto fifi;
	default:
		goto agn2;
	case ZDLE:		/* This is what we want. */
		break;
	}


	Rxhlen = 4;		/* Set default length */
	Rxframeind = c = noxrd7();
	switch (c) {
	case ZVBIN32:
		if ((Rxhlen = c = zdlread()) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 1;  c = zrbhd32(hdr); break;
	case ZBIN32:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 1;  c = zrbhd32(hdr); break;
	case ZVBINR32:
		if ((Rxhlen = c = zdlread()) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 2;  c = zrbhd32(hdr); break;
	case ZBINR32:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 2;  c = zrbhd32(hdr); break;
	case RCDO:
	case TIMEOUT:
	case ERROR:
		goto fifi;
	case ZVBIN:
		if ((Rxhlen = c = zdlread()) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 0;  c = zrbhdr(hdr); break;
	case ZBIN:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 0;  c = zrbhdr(hdr); break;
	case ZVHEX:
		if ((Rxhlen = c = zgethex()) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 0;  c = zrhhdr(hdr); break;
	case ZHEX:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 0;  c = zrhhdr(hdr); break;
	case CAN:
		goto gotcan;
	default:
		goto agn2;
	}
	for (n = Rxhlen; ++n < ZMAXHLEN; )	/* Clear unused hdr bytes */
		hdr[n] = 0;
	rxpos = hdr[ZP3] & 0377;
	rxpos = (rxpos<<8) + (hdr[ZP2] & 0377);
	rxpos = (rxpos<<8) + (hdr[ZP1] & 0377);
	rxpos = (rxpos<<8) + (hdr[ZP0] & 0377);
  fifi:
	switch (c) {
	case GOTCAN:
		c = ZCAN;
		/* **** FALL THRU TO **** */
	case ZNAK:
	case ZCAN:
	case ERROR:
	case TIMEOUT:
	case RCDO:
	case GCOUNT:
		/* **** FALL THRU TO **** */
	}
	if(c<=FRTYPES && c>=-5) zperr("ZErr %s", frametypes[c+FTOFFSET]);
	/* Use variable length headers if we got one */
#ifdef Z_DEBUG
	log("zgethdr: %s", frametypes[c+FTOFFSET]);
#endif
	if (c >= 0 && c <= FRTYPES && Rxframeind & 040)
		Usevhdrs = 1;
	return c;
}

/* Receive a binary style header (type and position) */
int zrbhdr(char *hdr)
{
	int c, n;
	unsigned short crc;

	if ((c = zdlread()) & ~0377)
		return c;
	Rxtype = c;
	crc = updcrc(c, 0);

	for (n=Rxhlen; --n >= 0; ++hdr) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = updcrc(c, crc);
		*hdr = c;
	}
	if ((c = zdlread()) & ~0377)
		return c;
	crc = updcrc(c, crc);
	if ((c = zdlread()) & ~0377)
		return c;
	crc = updcrc(c, crc);
	if (crc & 0xFFFF) {
		zperr(badcrc);
		return ERROR;
	}
	return Rxtype;
}

/* Receive a binary style header (type and position) with 32 bit FCS */
int zrbhd32(char *hdr)
{
	int c, n;
	unsigned long crc;

	if ((c = zdlread()) & ~0377)
		return c;
	Rxtype = c;
	crc = 0xFFFFFFFFL; crc = UPDC32(c, crc);

	for (n=Rxhlen; --n >= 0; ++hdr) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = UPDC32(c, crc);
		*hdr = c;
	}
	for (n=4; --n >= 0;) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = UPDC32(c, crc);
	}
	if (crc != 0xDEBB20E3) {
		zperr(badcrc);
		return ERROR;
	}
	return Rxtype;
}


/* Receive a hex style header (type and position) */
int zrhhdr(char *hdr)
{
	int c;
	unsigned short crc;
	int n;

	if ((c = zgethex()) < 0)
		return c;
	Rxtype = c;
	crc = updcrc(c, 0);

	for (n=Rxhlen; --n >= 0; ++hdr) {
		if ((c = zgethex()) < 0)
			return c;
		crc = updcrc(c, crc);
		*hdr = c;
	}
	if ((c = zgethex()) < 0)
		return c;
	crc = updcrc(c, crc);
	if ((c = zgethex()) < 0)
		return c;
	crc = updcrc(c, crc);
	if (crc & 0xFFFF) {
		zperr(badcrc); return ERROR;
	}
	c = GETCHAR(Rxtimeout);
	if (c < 0)
		return c;
	c = GETCHAR(Rxtimeout);
	if (c < 0)
		return c;
	return Rxtype;
}

/* Send a byte as two hex digits */
void zputhex(int c)
{
	static char	digits[]	= "0123456789abcdef";

	BUFCHAR(digits[(c&0xF0)>>4]&0x7f);
	BUFCHAR(digits[(c)&0xF]&0x7f);
}

/*
 * Send character c with ZMODEM escape sequence encoding.
 *  Escape XON, XOFF. Escape CR following @ (Telenet net escape)
 */
void zsendline(int c)
{

	/* Quick check for non control characters */
	if (c & 0140)
		BUFCHAR(lastsent = c)
	else {
		switch (c &= 0377) {
		case ZDLE:
			BUFCHAR(ZDLE);  BUFCHAR (lastsent = (c ^= 0100));
			break;
		case 021: case 023:
		case 0221: case 0223:
			BUFCHAR(ZDLE);  c ^= 0100;  BUFCHAR(lastsent = c);
			break;
		default:
			if (Zctlesc && ! (c & 0140)) {
				BUFCHAR(ZDLE);  c ^= 0100;
			}
			BUFCHAR(lastsent = c);
		}
	}
}

/* Decode two lower case hex digits into an 8 bit byte value */
int zgethex()
{
	int c;

	c = zgeth1();
	return c;
}

int zgeth1()
{
	int c, n;

	if ((c = noxrd7()) < 0)
		return c;
	n = c - '0';
	if (n > 9)
		n -= ('a' - ':');
	if (n & ~0xF)
		return ERROR;
	if ((c = noxrd7()) < 0)
		return c;
	c -= '0';
	if (c > 9)
		c -= ('a' - ':');
	if (c & ~0xF)
		return ERROR;
	c += (n<<4);
	return c;
}

/*
 * Read a byte, checking for ZMODEM escape encoding
 *  including CAN*5 which represents a quick abort
 */
int zdlread()
{
	int c;

  again:
	/* Quick check for non control characters */
	if ((c = GETCHAR(Rxtimeout)) & 0140)
		return c;
	switch (c) {
	case ZDLE:
		break;
	case 023:
	case 0223:
	case 021:
	case 0221:
		goto again;
	default:
		if (Zctlesc && !(c & 0140)) {
			goto again;
		}
		return c;
	}
  again2:
	if ((c = GETCHAR(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = GETCHAR(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = GETCHAR(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = GETCHAR(Rxtimeout)) < 0)
		return c;
	switch (c) {
	case CAN:
		return GOTCAN;
	case ZCRCE:
	case ZCRCG:
	case ZCRCQ:
	case ZCRCW:
		return (c | GOTOR);
	case ZRUB0:
		return 0177;
	case ZRUB1:
		return 0377;
	case 023:
	case 0223:
	case 021:
	case 0221:
		goto again2;
	default:
		if (Zctlesc && ! (c & 0140)) {
			goto again2;
		}
		if ((c & 0140) ==  0100)
			return (c ^ 0100);
		break;
	}
	return ERROR;
}

/*
 * Read a character from the modem line with timeout.
 *  Eat parity, XON and XOFF characters.
 */
int noxrd7()
{
	int c;

	for (;;) {
		if ((c = GETCHAR(Rxtimeout)) < 0)
			return c;
		switch (c &= 0177) {
		case XON:
		case XOFF:
			continue;
		default:
			if (Zctlesc && !(c & 0140))
				continue;
		case '\r':
		case '\n':
		case ZDLE:
			return c;
		}
	}
	/* NOTREACHED */
}

/* Store long integer pos in Txhdr */
void stohdr(pos)
	long pos;
{
	Txhdr[ZP0] = pos;
	Txhdr[ZP1] = pos>>8;
	Txhdr[ZP2] = pos>>16;
	Txhdr[ZP3] = pos>>24;
}

/* Recover a long integer from a header */
long rclhdr(char *hdr)
{
	long l;

	l = (hdr[ZP3] & 0377);
	l = (l << 8) | (hdr[ZP2] & 0377);
	l = (l << 8) | (hdr[ZP1] & 0377);
	l = (l << 8) | (hdr[ZP0] & 0377);
	return l;
}

/* End of zm.c */
