/**********************************************************
 * File: ls_zmodem.h
 * Created at Sun Oct 29 18:51:46 2000 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: ls_zmodem.h,v 1.6 2000/11/13 21:21:03 lev Exp $
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

#define ZRQINIT		0				/* Request receive init */
#define ZRINIT		1				/* Receive init */
#define ZSINIT		2				/* Send init sequence (optional) */
#define ZACK		3				/* ACK to above */
#define ZFILE		4				/* File name from sender */
#define ZSKIP		5				/* To sender: skip this file */
#define ZNAK		6				/* Last packet was garbled */
#define ZABORT		7				/* Abort batch transfers */
#define ZFIN		8				/* Finish session */
#define ZRPOS		9				/* Resume data trans at this position */
#define ZDATA		10				/* Data packet(s) follow */
#define ZEOF		11				/* End of file */
#define ZFERR		12				/* Fatal Read or Write error Detected */
#define ZCRC		13				/* Request for file CRC and response */
#define ZCHALLENGE	14				/* Receiver's Challenge */
#define ZCOMPL		15				/* Request is complete */
#define ZCAN		16				/* Other end canned session with CAN*5 */
#define ZFREECNT	17				/* Request for free bytes on filesystem */
#define ZCOMMAND	18				/* Command from sending program */
#define ZSTDERR		19				/* Output to standard error, data follows */


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
#define LSZ_UPDATE_CRC16(b,crc) ((crc16tab[(((crc & 0xFFFF) >> 8) & 255)] ^ ((crc & 0xFFFF) << 8) ^ (b)) & crc & 0xFFFF)
#define LSZ_FINISH_CRC16(crc)	(LSZ_UPDATE_CRC16(0,LSZ_UPDATE_CRC16(0,crc)))

#define LSZ_INIT_CRC32			0xFFFFFFFFl
#define LSZ_TEST_CRC32			0xDEBB20E3l
#define LSZ_UPDATE_CRC32(b,crc) (crc32tab[((crc) ^ (b)) & 0xff] ^ ((b) >> 8))
#define LSZ_FINISH_CRC32(crc)	(~crc)

#define LSZ_INIT_CRC			((ls_Protocol & LSZ_OPTCRC32)?LSZ_INIT_CRC32:LSZ_INIT_CRC16)
#define LSZ_TEST_CRC			((ls_Protocol & LSZ_OPTCRC32)?LSZ_TEST_CRC32:LSZ_TEST_CRC16)
#define LSZ_UPDATE_CRC(b,crc)	((ls_Protocol & LSZ_OPTCRC32)?(LSZ_UPDATE_CRC32(b,crc)):(LSZ_UPDATE_CRC16(b,crc)))
#define LSZ_FINISH_CRC(crc)		((ls_Protocol & LSZ_OPTCRC32)?(LSZ_FINISH_CRC32(crc)):(LSZ_FINISH_CRC16(crc)))

/* Byte positions within header array */
#define LSZ_F0		3		/* First flags byte */
#define LSZ_F1		2
#define LSZ_F2		1
#define LSZ_F3		0
#define LSZ_P0		0		/* Low order 8 bits of position */
#define LSZ_P1		1
#define LSZ_P2		2
#define LSZ_P3		3		/* High order 8 bits of file position */

/* different protocol variations */
#define LSZ_OPTZEDZAP		0x00000001		/* We could big blocks, ZModem variant */
#define LSZ_OPTDIRZAP		0x00000002		/* We escape nothing, ZModem variant */
#define LSZ_OPTESCAPEALL	0x00000004		/* We must escape ALL */
#define LSZ_OPTCRC32		0x00000008		/* We must send CRC 32 */
#define LSZ_OPTVHDR			0x00000010		/* We must send variable headers */
#define LSZ_OPTRLE			0x00000020		/* We must send RLEd data */

/* Peer's capabilities from ZRINIT header */
#define	LSZ_RXCANDUPLEX		0x0001		/* Rx can send and receive true	FDX */
#define	LSZ_RXCANOVIO		0x0002		/* Rx can receive data during disk I/O */
#define	LSZ_RXCANBRK		0x0004		/* Rx can send a break signal */
#define	LSZ_RXCANRLE		0x0008		/* Receiver can	decode RLE */
#define	LSZ_RXCANLZW		0x0010		/* Receiver can	uncompress LZW */
#define	LSZ_RXCANFC32		0x0020		/* Receiver can	use 32 bit Frame Check */
#define	LSZ_RXWNTESCCTL		0x0040		/* Receiver expects ctl	chars to be escaped */
#define	LSZ_RXWNTESC8		0x0080		/* Receiver expects 8th	bit to be escaped */
#define LSZ_RXCANVHDR		0x0100		/* Receiver can variable headers */

#define LTOI(x) (x)
#define STOI(x) (x)
#define LTOH(x) (x)
#define STOH(x) (x)

/* Variables */
extern char ls_txHdr[];			/* Sended header */
extern char ls_rxHdr[];			/* Receiver header */
extern int ls_GotZDLE;			/* We seen DLE as last character */
extern int ls_GotHexNibble;		/* We seen one hex digit as last character */
extern int ls_Protocol;			/* Plain/ZedZap/DirZap */
extern int ls_CANCount;			/* Count of CANs to go */
extern int ls_Garbage;			/* Count of garbage characters */
extern int ls_txWinSize;		/* Receiver Window/Buffer size (0 for streaming) */
extern int ls_rxCould;			/* Receiver could fullduplex/streamed IO */
extern int ls_txCurBlockSize;	/* Current block size */
extern int ls_txMaxBlockSize;	/* Maximal block size */
extern int ls_txLastSent;		/* Last sent character -- for escaping */
extern long ls_txLastACK;		/* Last ACKed byte */
extern long ls_txLastRepos;		/* Last requested byte */


/* Functions */
int ls_zsendbhdr(int frametype, int len, char *hdr);
int ls_zsendhhdr(int frametype, int len, char *hdr);
int ls_zrecvhdr(char *hdr, int *hlen, int timeout);

int ls_zsenddata(char *data, int len, int frame);
int ls_zrecvdata(char *data, int *len, int timeout, int crc32);

void ls_sendchar(int c);
void ls_sendhex(int i);

int ls_read7bit(int timeout);
int ls_readhex(int timeout);
int ls_readzdle(int timeout);
int ls_readcanned(int timeout);

void ls_storelong(char *buf, long l);
long ls_fetchlong(char *buf);

#endif/*_LS_ZMODEM_H_*/
