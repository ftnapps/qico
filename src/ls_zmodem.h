/**********************************************************
 * File: ls_zmodem.h
 * Created at Sun Oct 29 18:51:46 2000 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: ls_zmodem.h,v 1.2 2000/11/03 07:55:29 lev Exp $
 **********************************************************/
#ifndef _LS_ZMODEM_H_
#define _LS_ZMODEM_H_
/*

   ZModem file transfer protocol. Written from scratches.
   Support CRC16, CRC32, RLE Encoding, variable header, ZedZap (big blocks).
   Global variables, common functions.

*/

/* ZModem constants */

#define LSZ_MAXHLEN			16		/* Max header information length  NEVER CHANGE */
#define LSZ_ZMAXSPLEN		1024	/* Max subpacket length  NEVER CHANGE */

/* Special characters */
#define ZPAD		'*'				/* 052 Padding character begins frames */
#define ZDLE		030				/* Ctrl-X Zmodem escape - `ala BISYNC DLE */
#define ZDLEE		(ZDLE^0100)		/* Escaped ZDLE as transmitted */
#define ZDEL		0x7F			/* DEL character */
#define ZBIN		'A'				/* Binary frame indicator (CRC-16) */
#define ZHEX		'B'				/* HEX frame indicator */
#define ZBIN32		'C'				/* Binary frame with 32 bit FCS */
#define ZBINR32		'D'				/* RLE packed Binary frame with 32 bit FCS */
#define ZVBIN		'a'				/* Binary frame indicator (CRC-16) */
#define ZVHEX		'b'				/* HEX frame indicator */
#define ZVBIN32		'c'				/* Binary frame with 32 bit FCS */
#define ZVBINR32	'd'				/* RLE packed Binary frame with 32 bit FCS */
#define ZRESC		0176			/* RLE flag/escape character */

#define ZCRCE 'h'	/* CRC next, frame ends, header packet follows */
#define ZCRCG 'i'	/* CRC next, frame continues nonstop */
#define ZCRCQ 'j'	/* CRC next, frame continues, ZACK expected */
#define ZCRCW 'k'	/* CRC next, ZACK expected, end of frame */
#define ZRUB0 'l'	/* Translate to rubout 0177 */
#define ZRUB1 'm'	/* Translate to rubout 0377 */

/* Return codes -- pseudo-chracters */
#define LSZ_TIMEOUT		TIMEOUT	/* -2 */
#define LSZ_RCDO		RCDO	/* -3 */
#define LSZ_GCOUNT		GCOUNT	/* -4*/
#define LSZ_ERROR		ERROR	/* -5 */
#define LSZ_CAN			LSZ_GCOUNT
#define LSZ_NOHEADER	-6
#define LSZ_BADCRC		-7
#define LSZ_XONXOFF		-8
#define LSZ_CRCE		(ZCRCE | 0x0100)
#define LSZ_CRCG		(ZCRCG | 0x0100)
#define LSZ_CRCQ		(ZCRCQ | 0x0100)
#define LSZ_CRCW		(ZCRCW | 0x0100)


#define LSZ_INIT_CRC16 0x0000
#define LSZ_TEST_CRC16 0x0000

#define LSZ_INIT_CRC32 0xFFFFFFFF
#define LSZ_TEST_CRC32 0xDEBB20E3

#define LSZ_UPDATE_CRC16(b,crc) (crc16tab[(((crc) >> 8) & 255)] ^ ((crc) << 8) ^ (b))
#define LSZ_UPDATE_CRC32(b,crc) (crc32tab[((crc) ^ (b)) & 0xff] ^ ((b) >> 8))

#define LSZ_UPDATE_CRC(b,crc) (ls_txCRC32?(LSZ_UPDATE_CRC32(b,crc)):(LSZ_UPDATE_CRC16(b,crc)))

#define LSZ_ZEDZAP		0x0001		/* We could big blocks */
#define LSZ_DIRZAP		0x0002		/* We escape nothing */

#define LTOI(x) (x)
#define STOI(x) (x)
#define LTOH(x) (x)
#define STOH(x) (x)

/* Functions */
int zsendbhdr(int frametype, int len, char *hdr);
int zsendhhdr(int frametype, int len, char *hdr);
int ls_zrecvhdr(char *hdr, int timeout);
void ls_sendchar(int c);
void ls_sendhex(int i);
int ls_read7bit(int timeout);
int ls_readhex(int timeout);
int ls_readzdle(int timeout);

#endif/*_LS_ZMODEM_H_*/
