/**********************************************************
 * File: qipc_common.c
 * Created at Mon May 7 23:07:41 2001  by lev // lev@serebryakov.spb.ru
 * 
 * $Id: qipc_common.c,v 1.1 2001/05/21 20:03:09 lev Exp $
 **********************************************************/
#include "headers.h"
#include <stdarg.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>


/* Decode packet by signature */
int unpack_ipc_packet(unsigned char *data, int len, char *sig, ...)
{
	va_list args;
	long *pl;
	char *pc;
	int l;
	int rc = 0;

	va_start(args, sig);

	while(*sig) {
		switch(*sig) {
		case 's':		/* String */
			l = *data; data++; len--;
			if (l > len || data[l]) { va_end(args); return rc; }
			pc = va_arg(args,char *);
			memcpy(pc,data,l); len -= l; data += l;
			rc++;
			break;
		case 'd':
			pl = va_arg(args, long *);
			*pl = *data; data++; l--;
			break;
		case 'c':
		case 'a':
		}
		sig++;
	}
	va_end(args);
}