/* $Id: clserv.h,v 1.1 2004/01/12 21:41:56 sisoft Exp $ */
#ifndef __CLSERV_H__
#define __CLSERV_H__

#define CLS_UDP 1
#define CLS_SERVER 2

#define CLS_UI 0
#define CLS_LINE CLS_UDP
#define CLS_SERV_U CLS_SERVER
#define CLS_SERV_L CLS_SERVER|CLS_UDP

extern int cls_conn(int type);
extern void cls_close(int sock);
extern void cls_shutd(int sock);
extern int xsend(int sock,char *buf,int len);
extern int xrecv(int sock,char *buf,int len,int wait);

#endif
