/* $Id: clserv.h,v 1.2 2004/01/15 23:39:41 sisoft Exp $ */
#ifndef __CLSERV_H__
#define __CLSERV_H__

#define CLS_UDP 1
#define CLS_SERVER 2

#define CLS_UI 0
#define CLS_LINE CLS_UDP
#define CLS_SERV_U CLS_SERVER
#define CLS_SERV_L CLS_SERVER|CLS_UDP

typedef struct _cls_cl_t {
	struct _cls_cl_t *next;
	short id;
	int sock;
	char type;
} cls_cl_t;

extern int (*xsend_cb)(int sock,char *buf,int len);

extern int cls_conn(int type,char *port);
extern void cls_close(int sock);
extern void cls_shutd(int sock);
extern int xsend(int sock,char *buf,int len);
extern int xrecv(int sock,char *buf,int len,int wait);

#endif
