/**********************************************************
 * common definitions
 * $Id: defs.h,v 1.5 2004/02/19 23:36:39 sisoft Exp $
 **********************************************************/
#ifndef __DEFS_H__
#define __DEFS_H__

#define TRUE	1
#define FALSE	0

#define OK	0
#define ERROR	-5
#define TIMEOUT	-2
#define RCDO	-3
#define GCOUNT	-4

#define NOTTO(ch) ((ch)==ERROR || (ch)==RCDO || (ch)==EOF)
#define ISTO(ch) ((ch)==TIMEOUT || (ch)==OK)

#define XON	('Q'&037)
#define XOFF	('S'&037)

#define NUL	0x00
#define ACK	0x06
#define BEL	0x07
#define BS 	0x08
#define HT 	0x09
#define LF 	0x0a
#define VT 	0x0b
#define CR 	0x0d
#define DLE	0x10
#define CAN	0x18
#define ESC	0x1b

#endif
