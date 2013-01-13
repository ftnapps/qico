/* $Id: clserv.h,v 1.3 2005/05/16 11:17:30 mitry Exp $
 *
 * $Log: clserv.h,v $
 * Revision 1.3  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.2  2005/04/05 09:25:29  mitry
 * Fix for tty device name > 8 symbols
 *
 */

#ifndef __CLSERV_H__
#define __CLSERV_H__

#define DEF_SERV_PORT 60178

#define CLS_UDP 1
#define CLS_SERVER 2

#define CLS_UI 0
#define CLS_LINE CLS_UDP
#define CLS_SERV_U CLS_SERVER
#define CLS_SERV_L CLS_SERVER|CLS_UDP

/* hack for some older systems */
#ifndef MSG_WAITALL
#define MSG_WAITALL 0
#endif

#define QCC_TTYLEN	32		/* tty name length */

typedef struct _cls_cl_t {
	struct _cls_cl_t *next;
	unsigned char *auth;
	short id;
	int sock;
	char type;
} cls_cl_t;

typedef struct _cls_ln_t {
	struct _cls_ln_t *next;
	unsigned short id;
	struct sockaddr *addr;
	int emsilen;
	char *emsi;
} cls_ln_t;

typedef int (*xsend_cb_t)(int, const char *, size_t);

extern xsend_cb_t xsend_cb;

int	cls_conn(int, const char *, const char *);
void	cls_close(int);
void	cls_shutd(int);
int	xsendto(int, const char *, size_t, struct sockaddr *);
int	xsend(int, const char *, size_t);
int	xrecv(int, char *, size_t, int);

#endif
