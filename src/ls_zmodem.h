#ifndef _LS_ZMODEM_H_
#define _LS_ZMODEM_H_
/*
   ZModem file transfer protocol. Written from scratches.
   Support CRC16, CRC32, variable header, ZedZap (big blocks) and DirZap.
   Global variables, common functions.
*/
/* $Id: ls_zmodem.h,v 1.1.1.1 2003/07/12 21:26:54 sisoft Exp $ */

/* Dirty hack */
#define LSZ_TRUSTZFINS		3		/* We trust only in MANY ZFINs during initialization */
 
/* ZModem constants */

#define LSZ_MAXHLEN			16		/* Max header information length  NEVER CHANGE */
#define LSZ_MAXDATALEN		1024	/* Max subpacket length  NEVER CHANGE (for ZModem, really could be 8192 for ZedZap/DirZap) */
#define LSZ_MAXATTNLEN		32		/* Max length of Attn string */

/* Special characters */
#define ZPAD		'*'				/* 052 Padding character begins frames */
#define ZDLE		0x18			/* Ctrl-X Zmodem escape - `ala BISYNC DLE */
#define ZDLEE		(ZDLE^0x40)		/* Escaped ZDLE as transmitted */
#define ZDEL		0x7f			/* DEL character */
#define ZBIN		'A'				/* Binary frame indicator (CRC-16) */
#define ZHEX		'B'				/* HEX frame indicator */
#define ZBIN32		'C'				/* Binary frame with 32 bit FCS */
#define ZBINR32		'D'				/* RLE packed Binary frame with 32 bit FCS */
#define ZVBIN		'a'				/* Binary frame indicator (CRC-16) and v.header */
#define ZVHEX		'b'				/* HEX frame indicator and v.header */
#define ZVBIN32		'c'				/* Binary frame with 32 bit FCS and v.header */
#define ZVBINR32	'd'				/* RLE packed Binary frame with 32 bit FCS and v.header */
#define ZRESC		0x7e			/* RLE flag/escape character */

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
#define ZFERR		12				/* Fatal Read or Write error Detected (we use for refuse -- suspend) */
#define ZCRC		13				/* Request for file CRC and response */
#define ZCHALLENGE	14				/* Receiver's Challenge */
#define ZCOMPL		15				/* Request is complete */
#define ZCAN		16				/* Other end canned session with CAN*5 */
#define ZFREECNT	17				/* Request for free bytes on filesystem (OK, we always send unlimited) */
#define ZCOMMAND	18				/* Command from sending program (NOT SUPPORTED!) */
#define ZSTDERR		19				/* Output to standard error, data follows (NOT SUPPORTED!) */
#define LSZ_MAXFRAME	ZSTDERR


#define ZCRCE 'h'	/* CRC next, frame ends, header packet follows */
#define ZCRCG 'i'	/* CRC next, frame continues nonstop */
#define ZCRCQ 'j'	/* CRC next, frame continues, ZACK expected */
#define ZCRCW 'k'	/* CRC next, ZACK expected, end of frame */
#define ZRUB0 'l'	/* Translate to rubout 0177 */
#define ZRUB1 'm'	/* Translate to rubout 0377 */

/* Return codes -- pseudo-chracters */
#define LSZ_OK			OK		/* 0 */
#define LSZ_TIMEOUT		TIMEOUT	/* -2 */
#define LSZ_RCDO		RCDO	/* -3 */
#define LSZ_CAN			GCOUNT	/* -4*/
#define LSZ_ERROR		ERROR	/* -5 */
#define LSZ_NOHEADER	-6
#define LSZ_BADCRC		-7
#define LSZ_XONXOFF		-8
#define LSZ_CRCE		(ZCRCE | 0x0100)
#define LSZ_CRCG		(ZCRCG | 0x0100)
#define LSZ_CRCQ		(ZCRCQ | 0x0100)
#define LSZ_CRCW		(ZCRCW | 0x0100)


#define LSZ_INIT_CRC16 CRC16USD_INIT
#define LSZ_TEST_CRC16 CRC16USD_TEST
#define LSZ_UPDATE_CRC16(b,crc) CRC16USD_UPDATE(b,crc)
#define LSZ_FINISH_CRC16(crc)	CRC16USD_FINISH(crc)

#define LSZ_INIT_CRC32			CRC32_INIT
#define LSZ_TEST_CRC32			CRC32_TEST
#define LSZ_UPDATE_CRC32(b,crc) CRC32_UPDATE(b,crc)
#define LSZ_FINISH_CRC32(crc)	CRC32_FINISH(crc)

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
#define LSZ_OPTZEDZAP		0x00000001		/* We could big blocks, ZModem variant (ZedZap) */
#define LSZ_OPTDIRZAP		0x00000002		/* We escape nothing, ZModem variant (DirZap) */
#define LSZ_OPTESCAPEALL	0x00000004		/* We must escape ALL contorlo characters */
#define LSZ_OPTCRC32		0x00000008		/* We must send CRC 32 */
#define LSZ_OPTVHDR			0x00000010		/* We must send variable headers */
#define LSZ_OPTRLE			0x00000020		/* We must send RLEd data (NOT SUPPORTED)! */
#define LSZ_OPTESC8			0x00000040		/* We must escape all 8-bit data (NOT SUPPORTED!) */
#define LSZ_OPTSKIPGUARD	0x00000080		/* We use double-skip guard */
#define LSZ_OPTFIRSTBATCH	0x00000100		/* It is first batch -- trust in first ZFIN */

/* Peer's capabilities from ZRINIT header */
#define	LSZ_RXCANDUPLEX		0x0001		/* Receiver can send and receive true	FDX */
#define	LSZ_RXCANOVIO		0x0002		/* Receiver can receive data during disk I/O */
#define	LSZ_RXCANBRK		0x0004		/* Receiver can send a break signal */
#define	LSZ_RXCANRLE		0x0008		/* Receiver can	decode RLE */
#define	LSZ_RXCANLZW		0x0010		/* Receiver can	uncompress LZW */
#define	LSZ_RXCANFC32		0x0020		/* Receiver can	use 32 bit Frame Check */
#define	LSZ_RXWNTESCCTL		0x0040		/* Receiver expects ctl	chars to be escaped */
#define	LSZ_RXWNTESC8		0x0080		/* Receiver expects 8th	bit to be escaped */
#define LSZ_RXCANVHDR		0x0100		/* Receiver can variable headers */

/* Our requirenets, for ZF0 of ZSINIT frame */
#define LSZ_TXWNTESCCTL 	0x0040		/* Transmitter expects ctl chars to be escaped */
#define LSZ_TXWNTESC8	 	0x0080		/* Transmitter expects 8th bit to be escaped (NOT SUPPORTED) */

/* Conversion options (ZF0 in ZFILE frame) */
#define LSZ_CONVBIN		1		/* Binary transfer - inhibit conversion */
#define LSZ_CONVCNL		2		/* Convert NL to local end of line convention */
#define LSZ_CONVRECOV	3		/* Resume interrupted file transfer -- binary */

/* File management options (ZF1 in ZFILE frame) */
#define LSZ_MSKNOLOC	0x80	/* Skip file if not present at rx */
#define LSZ_MMASK		0x1f	/* Mask for the choices below */
#define LSZ_MNEWL		1		/* Transfer if source newer or longer */
#define LSZ_MCRC		2		/* Transfer if different file CRC or length */
#define LSZ_MAPND		3		/* Append contents to existing file (if any) */
#define LSZ_MCLOB		4		/* Replace existing file */
#define LSZ_MNEW		5		/* Transfer if source newer */
#define LSZ_MDIFF		6		/* Transfer if dates or lengths different */
#define LSZ_MPROT		7		/* Protect destination file */
#define LSZ_MCHNG		8		/* Change filename if destination exists */

/* Compression options (ZF2 in ZFILE frame) */
#define LSZ_COMPNO		0		/* no compression */
#define LSZ_COMPLZW		1		/* Lempel-Ziv compression */
#define LSZ_COMPRLE		3		/* Run Length encoding */

/* Extended options (ZF3 in ZFILE frame) */
#define LSZ_EXTCANVHDR	0x01	/* Variable headers OK */
#define LSZ_EXTWOVR 	0x04	/* Receiver window override */
#define LSZ_EXTSPARS	0x40	/* Sparse file support (NOT SUPPORTED!) */

/* Stubs for byte order conversion */
#define LTOI(x) (x)
#define STOI(x) (x)
#define LTOH(x) (x)
#define STOH(x) (x)

#define CHAT_DONE 0
#define CHAT_DATA 1

/* ZModem state */

/* Common variables */
extern byte ls_txHdr[];			/* Sended header */
extern byte ls_rxHdr[];			/* Receiver header */
extern int ls_GotZDLE;			/* We seen DLE as last character */
extern int ls_GotHexNibble;		/* We seen one hex digit as last character */
extern int ls_Protocol;			/* Plain/ZedZap/DirZap and other options */
extern int ls_CANCount;			/* Count of CANs to go */
extern int ls_Garbage;			/* Count of garbage characters */
extern int ls_SerialNum;		/* Serial number of file -- for Double-skip protection */
extern int ls_HeaderTimeout;	/* Timeout for headers */
extern int ls_DataTimeout;		/* Timeout for data blocks */
extern int ls_MaxBlockSize;		/* Maximum block size */
int ls_SkipGuard;			/* double-skip protection on/off */

/* Variables to control sender */
extern int ls_txWinSize;		/* Receiver Window/Buffer size (0 for streaming) */
extern int ls_rxCould;			/* Receiver could fullduplex/streamed IO (from ZRINIT) */
extern int ls_txCurBlockSize;	/* Current block size */
extern int ls_txLastSent;		/* Last sent character -- for escaping */
extern long ls_txLastACK;		/* Last ACKed byte */
extern long ls_txLastRepos;		/* Last requested byte */
extern long ls_txReposCount;	/* Count of REPOSes on one position */
extern long ls_txGoodBlocks;	/* Good blocks sent */

typedef struct _ZFILEINFO {
	char name[MAX_PATH];
	unsigned long size;
	time_t mtime;
	unsigned long filesleft;
	unsigned long bytesleft;
} ZFILEINFO;

/* Functions */
int ls_zsendbhdr(int frametype, int len, byte *hdr);
int ls_zsendhhdr(int frametype, int len, byte *hdr);
int ls_zrecvhdr(byte *hdr, int *hlen, int timeout);

int ls_zsenddata(byte *data, int len, int frame);
int ls_zrecvdata16(byte *data, int *len, int timeout);
int ls_zrecvdata32(byte *data, int *len, int timeout);
#define ls_zrecvdata(data,len,timeout,crc32) ((crc32)?ls_zrecvdata32(data,len,timeout):ls_zrecvdata16(data,len,timeout))

void ls_sendchar(int c);
void ls_sendhex(int i);

int ls_read7bit(int *timeout);
int ls_readhex(int *timeout);
int ls_readzdle(int *timeout);
int ls_readcanned(int *timeout);

void ls_storelong(byte *buf, long l);
long ls_fetchlong(unsigned char *buf);

/* Top-level functions */
/* Init sender */
int ls_zinitsender(int protocol, int baud, int window, char *attstr);
/* Send one file */
int ls_zsendfile(ZFILEINFO *f, unsigned long sernum);
/* Done sender -- finish sending */
int ls_zdonesender();

void ls_zabort();

/* Init receiver, return ZFILE, ZFIN or error. Leave ZFILE data frame in buffer */
int ls_zinitreceiver(int protocol, int baud, int window, ZFILEINFO *f);
/* Receive one file form position POS */
int ls_zrecvfile(int pos);
/* Prepare to receive next file (and, any be skeip or refuse current file) */
int ls_zrecvfinfo(ZFILEINFO *f, int frame, int first);
/* Done receiver */
int ls_zdonereceiver();

//chat
void z_devsend_c(int buffr);
void z_devrecv_c(unsigned char c,int flushed);

int z_devfree();
int z_devsend(unsigned char *data,unsigned short len);


/* Debug logging */
#ifdef NEED_DEBUG
extern char *LSZ_FRAMETYPES[];
#define LSZ_FTOFFSET	8
#endif/*NEED_DEBUG*/

#endif/*_LS_ZMODEM_H_*/
