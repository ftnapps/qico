/**********************************************************
 * File: zmodem.h
 * Created at Fri Jul 16 17:24:54 1999 by pk // aaz@ruxy.org.ru
 * Manifest constants for ZMODEM
 * $Id: zmodem.h,v 1.2 2000/10/07 13:38:42 lev Exp $
 **********************************************************/
/*
 *    Copyright 1993 Omen Technology Inc All Rights Reserved
 */
#ifndef __ZMODEM_H__
#define __ZMODEM_H__

#define ZMAXBLOCK   8192

#define ZPAD '*'	/* 052 Padding character begins frames */
#define ZDLE 030	/* Ctrl-X Zmodem escape - `ala BISYNC DLE */
#define ZDLEE (ZDLE^0100)	/* Escaped ZDLE as transmitted */
#define ZBIN 'A'	/* Binary frame indicator (CRC-16) */
#define ZHEX 'B'	/* HEX frame indicator */
#define ZBIN32 'C'	/* Binary frame with 32 bit FCS */
#define ZBINR32 'D'	/* RLE packed Binary frame with 32 bit FCS */
#define ZVBIN 'a'	/* Binary frame indicator (CRC-16) */
#define ZVHEX 'b'	/* HEX frame indicator */
#define ZVBIN32 'c'	/* Binary frame with 32 bit FCS */
#define ZVBINR32 'd'	/* RLE packed Binary frame with 32 bit FCS */
#define ZRESC	0176	/* RLE flag/escape character */
#define ZMAXHLEN 16	/* Max header information length  NEVER CHANGE */
#define ZMAXSPLEN 1024	/* Max subpacket length  NEVER CHANGE */

/* Frame types (see array "frametypes" in zm.c) */
#define ZRQINIT	0	/* Request receive init */
#define ZRINIT	1	/* Receive init */
#define ZSINIT 2	/* Send init sequence (optional) */
#define ZACK 3		/* ACK to above */
#define ZFILE 4		/* File name from sender */
#define ZSKIP 5		/* To sender: skip this file */
#define ZNAK 6		/* Last packet was garbled */
#define ZABORT 7	/* Abort batch transfers */
#define ZFIN 8		/* Finish session */
#define ZRPOS 9		/* Resume data trans at this position */
#define ZDATA 10	/* Data packet(s) follow */
#define ZEOF 11		/* End of file */
#define ZFERR 12	/* Fatal Read or Write error Detected */
#define ZCRC 13		/* Request for file CRC and response */
#define ZCHALLENGE 14	/* Receiver's Challenge */
#define ZCOMPL 15	/* Request is complete */
#define ZCAN 16		/* Other end canned session with CAN*5 */
#define ZFREECNT 17	/* Request for free bytes on filesystem */
#define ZCOMMAND 18	/* Command from sending program */
#define ZSTDERR 19	/* Output to standard error, data follows */

/* ZDLE sequences */
#define ZCRCE 'h'	/* CRC next, frame ends, header packet follows */
#define ZCRCG 'i'	/* CRC next, frame continues nonstop */
#define ZCRCQ 'j'	/* CRC next, frame continues, ZACK expected */
#define ZCRCW 'k'	/* CRC next, ZACK expected, end of frame */
#define ZRUB0 'l'	/* Translate to rubout 0177 */
#define ZRUB1 'm'	/* Translate to rubout 0377 */

/* zdlread return values (internal) */
/* -1 is general error, -2 is timeout */
#define GOTOR 0400
#define GOTCRCE (ZCRCE|GOTOR)	/* ZDLE-ZCRCE received */
#define GOTCRCG (ZCRCG|GOTOR)	/* ZDLE-ZCRCG received */
#define GOTCRCQ (ZCRCQ|GOTOR)	/* ZDLE-ZCRCQ received */
#define GOTCRCW (ZCRCW|GOTOR)	/* ZDLE-ZCRCW received */
#define GOTCAN	(GOTOR|030)	/* CAN*5 seen */

/* Byte positions within header array */
#define ZF0	3	/* First flags byte */
#define ZF1	2
#define ZF2	1
#define ZF3	0
#define ZP0	0	/* Low order 8 bits of position */
#define ZP1	1
#define ZP2	2
#define ZP3	3	/* High order 8 bits of file position */

/* Bit Masks for ZRINIT flags byte ZF0 */
#define CANFDX	01	/* Rx can send and receive true FDX */
#define CANOVIO	02	/* Rx can receive data during disk I/O */
#define CANBRK	04	/* Rx can send a break signal */
#define CANRLE	010	/* Receiver can decode RLE */
#define CANLZW	020	/* Receiver can uncompress */
#define CANFC32	040	/* Receiver can use 32 bit Frame Check */
#define ESCCTL 0100	/* Receiver expects ctl chars to be escaped */
#define ESC8   0200	/* Receiver expects 8th bit to be escaped */

/* Bit Masks for ZRINIT flags byte ZF1 */
#define CANVHDR	01	/* Variable headers OK */

/* Parameters for ZSINIT frame */
#define ZATTNLEN 32	/* Max length of attention string */
#define ALTCOFF ZF1	/* Offset to alternate canit string, 0 if not used */
/* Bit Masks for ZSINIT flags byte ZF0 */
#define TESCCTL 0100	/* Transmitter expects ctl chars to be escaped */
#define TESC8   0200	/* Transmitter expects 8th bit to be escaped */

/* Parameters for ZFILE frame */
/* Conversion options one of these in ZF0 */
#define ZCBIN	1	/* Binary transfer - inhibit conversion */
#define ZCNL	2	/* Convert NL to local end of line convention */
#define ZCRESUM	3	/* Resume interrupted file transfer */
/* Management include options, one of these ored in ZF1 */
#define ZMSKNOLOC	0200	/* Skip file if not present at rx */
/* Management options, one of these ored in ZF1 */
#define ZMMASK	037	/* Mask for the choices below */
#define ZMNEWL	1	/* Transfer if source newer or longer */
#define ZMCRC	2	/* Transfer if different file CRC or length */
#define ZMAPND	3	/* Append contents to existing file (if any) */
#define ZMCLOB	4	/* Replace existing file */
#define ZMNEW	5	/* Transfer if source newer */
	/* Number 5 is alive ... */
#define ZMDIFF	6	/* Transfer if dates or lengths different */
#define ZMPROT	7	/* Protect destination file */
#define ZMCHNG	8	/* Change filename if destination exists */
/* Transport options, one of these in ZF2 */
#define ZTLZW	1	/* Lempel-Ziv compression */
#define ZTRLE	3	/* Run Length encoding */
/* Extended options for ZF3, bit encoded */
#define ZXSPARS	64	/* Encoding for sparse file operations */
#define ZCANVHDR	01	/* Variable headers OK */
/* Receiver window size override */
#define ZRWOVR 4	/* byte position for receive window override/256 */

/* Parameters for ZCOMMAND frame ZF0 (otherwise 0) */
#define ZCACK1	1	/* Acknowledge, then do command */

/* Globals used by ZMODEM functions */
extern int Rxframeind;	/* ZBIN ZBIN32, or ZHEX type of frame */
extern int Rxtype;		/* Type of header received */
extern int Rxcount;		/* Count of data bytes received */
extern int Rxtimeout;	/* Tenths of seconds to wait for something */
extern char Rxhdr[ZMAXHLEN];	/* Received header */
extern char Txhdr[ZMAXHLEN];	/* Transmitted header */
extern long Rxpos;	/* Received file position */
extern long Txpos;	/* Transmitted file position */
extern int Txfcs32;		/* TURE means send binary frames with 32 bit FCS */
extern int Crc32t;		/* Display flag indicating 32 bit CRC being sent */
extern int Crc32r;		/* Display flag indicating 32 bit CRC being received */
extern int Znulls;		/* Number of nulls to send at beginning of ZDATA hdr */
extern char Attn[ZATTNLEN+1];	/* Attention string rx sends to tx on err */
extern char *Altcan;	/* Alternate canit string */
extern int Zctlesc;            /* Encode control characters */
extern int Zrwindow;    /* RX window size (controls garbage count) */
extern unsigned Effbaud;
extern char badcrc[];
extern int Usevhdrs;		/* Use variable length headers */

extern void zsbhdr(int len, int type, register char *hdr);
extern void zsbh32(int len, register char *hdr, int type, int flavour);
extern void zshhdr(int len, int type, register char *hdr);
extern void zsdata(register char *buf, int length, int frameend);
extern void zsda32(register char *buf, int length, int frameend);
extern int zrdata(register char *buf, int length);
extern int zrdat32(register char *buf, int length);
extern void garbitch(void);
extern int zgethdr(char *hdr);
extern int zrbhdr(register char *hdr);
extern int zrbhd32(register char *hdr);
extern int zrhhdr(char *hdr);
extern void zputhex(register int c);
extern void zsendline(register int c);
extern int zgethex(void);
extern int zgeth1(void);
extern int zdlread(void);
extern int noxrd7(void);
extern void stohdr(long pos);
extern long rclhdr(register char *hdr);
extern char *zstatus(int s);
/* zmr.c */
extern void zsdar32(char *buf, int length, int frameend);
extern int zrdatr32(register char *buf, int length);
extern int zapok;


#undef DSZ

#define UPDC32(octet, crc) (crc32tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))
#define updcrc(cp, crc) (crc16tab[(((crc) >> 8) & 255)] ^ ((crc) << 8) ^ (cp))

/* #define flushmo() { fflush(stdin);fflush(stdout); } */
#define zperr sline

#define ZBUFFER       16384
extern char *zbuffer;
extern int  zlength;

#define BUFCLEAR()  zlength=0
#define BUFCHAR(c)  { if(zlength>=ZBUFFER) BUFFLUSH(); zbuffer[zlength++]=(c); }
#define BUFFLUSH()  { PUTBLK(zbuffer, zlength);zlength=0; }

#endif
/* End of zmodem.h */
