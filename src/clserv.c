/**********************************************************
 * client/server tools
 * $Id: clserv.c,v 1.1 2004/01/12 21:41:56 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "clserv.h"

int cls_conn(int type)
{
	int rc,f;
	struct sockaddr_in sa;
	sa.sin_family=AF_INET;
	sa.sin_port=htons(60178);
	rc=socket(AF_INET,type&CLS_UDP?SOCK_DGRAM:SOCK_STREAM,0);
    	if(rc<0)return rc;
	    else {
		if(type&CLS_SERVER) {
			sa.sin_addr.s_addr=INADDR_ANY;
			f=fcntl(rc,F_GETFL,0);
			if(f>=0)fcntl(rc,F_SETFL,f|O_NONBLOCK);
			if(bind(rc,(struct sockaddr*)&sa,sizeof(sa))<0)return -1;
			if(!(type&CLS_UDP))if(listen(rc,4)<0)return -1;
			return rc;
		}
		inet_pton(AF_INET,"127.0.0.1",&sa.sin_addr);
		if(connect(rc,(struct sockaddr*)&sa,sizeof(sa))<0)return -1;
		    else return rc;
	}
	return -1;
}

void cls_close(int sock)
{
	if(sock>=0)close(sock);
}

void cls_shutd(int sock)
{
	if(sock>=0)shutdown(sock,3);
	cls_close(sock);
}

int xsend(int sock,char *buf,int len)
{
	int rc;
	unsigned short l=len+2;
	rc=send(sock,&l,2,MSG_DONTWAIT);
	if(rc<0)return rc;
	return send(sock,buf,len,0);
}

int xrecv(int sock,char *buf,int len,int wait)
{
	int rc;
	unsigned short l=0;
	rc=recv(sock,&l,2,MSG_PEEK|(wait?MSG_WAITALL:MSG_DONTWAIT));
	if(rc<=0)return rc;
	if(rc==2&&l>0) {
		rc=recv(sock,&l,2,MSG_WAITALL);
		if(rc<2)return -1;
		if(l>len)l=len;
		return recv(sock,buf,l,MSG_WAITALL);
	}
	return 0;
}
