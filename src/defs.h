/**********************************************************
 * common definitions
 * $Id: defs.h,v 1.1.1.1 2003/07/12 21:26:32 sisoft Exp $
 **********************************************************/
#ifndef __CONSTQWE_H__
#define __CONSTQWE_H__

#define TRUE            1
#define FALSE           0

/*--------------------------------------------------------------------------*/
/* Miscellaneous definitions                                                */
/*--------------------------------------------------------------------------*/

#define OK				0
#define ERROR			-5
#define TIMEOUT			-2
#define RCDO			-3
#define GCOUNT			-4

#define NOTTO(ch) ((ch)==ERROR || (ch)==RCDO || (ch)==EOF)
#define ISTO(ch) ((ch)==TIMEOUT || (ch)==OK)

#define XON				('Q'&037)
#define XOFF			('S'&037)
#define CPMEOF			('Z'&037)

/*--------------------------------------------------------------------------*/
/* ASCII MNEMONICS                                                          */
/*--------------------------------------------------------------------------*/

#define NUL	0x00
#define SOH	0x01
#define STX	0x02
#define ETX	0x03
#define EOT	0x04
#define ENQ	0x05
#define ACK	0x06
#define BEL	0x07
#define BS 	0x08
#define HT 	0x09
#define LF 	0x0a
#define VT 	0x0b
#define FF 	0x0c
#define CR 	0x0d
#define SO 	0x0e
#define SI 	0x0f
#define DLE	0x10
#define DC1	0x11
#define DC2	0x12
#define DC3	0x13
#define DC4	0x14
#define NAK	0x15
#define SYN	0x16
#define ETB	0x17
#define CAN	0x18
#define EM	0x19
#define SUB	0x1a
#define ESC	0x1b
#define FS	0x1c
#define GS	0x1d
#define RS	0x1e
#define US	0x1f

#endif
