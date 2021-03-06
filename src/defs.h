/**********************************************************
 * common definitions
 * $Id: defs.h,v 1.5 2005/05/16 20:33:46 mitry Exp $
 **********************************************************/
#ifndef __DEFS_H__
#define __DEFS_H__

#undef TRUE
#define TRUE	1
#undef FALSE
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

#ifndef HAVE_STDXXX_FILENO
#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2
#endif

#define LARGE_STRING	((MAX_STRING*8) + 1)

#define C0(c) ((( c ) >= ' ' && ( c ) != 127 ) ? (c) : '.' )

/* daemon modes */
#define DM_NONE		-1
#define DM_ANSWER	0
#define DM_DAEMON	1
#define DM_NODELIST	2
#define DM_CONFIG	3
#define DM_CALL		12

#endif
