/**********************************************************
 * File: zmr.c
 * Created at Fri Jul 16 17:22:42 1999 by pk // aaz@ruxy.org.ru
 * ZMODEM RLE compression and decompression functions
 * $Id: zmr.c,v 1.2 2000/11/26 13:17:35 lev Exp $
 **********************************************************/
#include "headers.h"
#include "defs.h"
#include "zmodem.h"
#include "qipc.h"

/* Send data subpacket RLE encoded with 32 bit FCS */
void zsdar32(buf, length, frameend)
char *buf;
{
	register int c, l, n;
	register unsigned long crc;

	crc = 0xFFFFFFFFL;  l = *buf++ & 0377;
	if (length == 1) {
		zsendline(l); crc = UPDC32(l, crc);
		if (l == ZRESC) {
			zsendline(1); crc = UPDC32(1, crc);
		}
	} else {
		for (n = 0; --length >= 0; ++buf) {
			if ((c = *buf & 0377) == l && n < 126 && length>0) {
				++n;  continue;
			}
			switch (n) {
			case 0:
				zsendline(l);
				crc = UPDC32(l, crc);
				if (l == ZRESC) {
					zsendline(0100); crc = UPDC32(0100, crc);
				}
				l = c; break;
			case 1:
				if (l != ZRESC) {
					zsendline(l); zsendline(l);
					crc = UPDC32(l, crc);
					crc = UPDC32(l, crc);
					n = 0; l = c; break;
				}
				/* **** FALL THRU TO **** */
			default:
				zsendline(ZRESC); crc = UPDC32(ZRESC, crc);
				if (l == 040 && n < 34) {
					n += 036;
					zsendline(n); crc = UPDC32(n, crc);
				}
				else {
					n += 0101;
					zsendline(n); crc = UPDC32(n, crc);
					zsendline(l); crc = UPDC32(l, crc);
				}
				n = 0; l = c; break;
			}
		}
	}
	BUFCHAR(ZDLE);BUFCHAR(frameend);
	crc = UPDC32(frameend, crc);

	crc = ~crc;
	for (length=4; --length >= 0;) {
		zsendline((int)crc);  crc >>= 8;
	}
}


/* Receive data subpacket RLE encoded with 32 bit FCS */
int zrdatr32(buf, length)
register char *buf;
{
	register int c;
	register unsigned long crc;
	register char *end;
	register int d;

	crc = 0xFFFFFFFFL;  Rxcount = 0;  end = buf + length;
	d = 0;	/* Use for RLE decoder state */
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
				zperr("Bad data subpacket");
				return c;
			}
		}
		crc = UPDC32(c, crc);
		switch (d) {
		case 0:
			if (c == ZRESC) {
				d = -1;  continue;
			}
			*buf++ = c;  continue;
		case -1:
			if (c >= 040 && c < 0100) {
				d = c - 035; c = 040;  goto spaces;
			}
			if (c == 0100) {
				d = 0;
				*buf++ = ZRESC;  continue;
			}
			d = c;  continue;
		default:
			d -= 0100;
			if (d < 1)
				goto badpkt;
spaces:
			if ((buf + d) > end)
				goto badpkt;
			while ( --d >= 0)
				*buf++ = c;
			d = 0;  continue;
		}
	}
badpkt:
	zperr("Data subpacket too long");
	return ERROR;
}

/* End of zmr.c */
