/**********************************************************
 * client/server tools
 * $Id: clserv.c,v 1.10 2004/03/15 01:19:30 sisoft Exp $
 **********************************************************/
#include "headers.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include "clserv.h"

int (*xsend_cb)(int sock,char *buf,size_t len)=NULL;

int cls_conn(int type,char *port)
{
	int rc,f=1;
	struct sockaddr_in sa;
	struct servent *se;
	sa.sin_family=AF_INET;
	if(port&&atoi(port))sa.sin_port=htons(atoi(port));
	    else if(port&&(se=getservbyname(port,(type&CLS_UDP)?"udp":"tcp")))sa.sin_port=se->s_port;
	      else if(!port&&(se=getservbyname("qicoui",(type&CLS_UDP)?"udp":"tcp")))sa.sin_port=se->s_port;
		else if(!port)sa.sin_port=htons(DEF_SERV_PORT);
		    else {errno=EINVAL;return -1;}
	rc=socket(AF_INET,type&CLS_UDP?SOCK_DGRAM:SOCK_STREAM,0);
    	if(rc<0)return rc;
	sa.sin_addr.s_addr=htonl((type&CLS_UDP)?INADDR_LOOPBACK:INADDR_ANY);
	if(type&CLS_SERVER) {
		if(setsockopt(rc,SOL_SOCKET,SO_REUSEADDR,&f,sizeof(f))<0)return -1;
		f=fcntl(rc,F_GETFL,0);
		if(f>=0)fcntl(rc,F_SETFL,f|O_NONBLOCK);
		if(bind(rc,(struct sockaddr*)&sa,sizeof(sa))<0)return -1;
		if(!(type&CLS_UDP)) {
			if(listen(rc,4)<0)return -1;
			xsend_cb=xsend;
			return rc;
		}
		xsend_cb=xsend;
		return rc;
	}
	if(connect(rc,(struct sockaddr*)&sa,sizeof(sa))<0)return -1;
	xsend_cb=xsend;
	return rc;
}

void cls_close(int sock)
{
	if(sock>=0)close(sock);
}

void cls_shutd(int sock)
{
#ifdef HAVE_SHUTDOWN
	if(sock>=0)shutdown(sock,3);
#endif
	cls_close(sock);
}

int xsendto(int sock,char *buf,size_t len,struct sockaddr *to)
{
	int rc;
	char *b;
	unsigned short l=H2I16(len);
	if(sock<0){errno=EBADF;return -1;}
	if(!len)return 0;
	b=malloc(len+2);
	if(!b){errno=ENOMEM;return -1;}
	STORE16(b,l);
	memcpy(b+2,buf,len);
	rc=sendto(sock,b,len+2,0,to,sizeof(struct sockaddr));
	xfree(b);
	return rc;
}

int xsend(int sock,char *buf,size_t len)
{
	return xsendto(sock,buf,len,NULL);
}

int xrecv(int sock,char *buf,size_t len,int wait)
{
	int rc;
	fd_set rfd;
	unsigned short l=0;
	struct timeval tv;
	if(sock<0){errno=EBADF;return -1;}
	if(!wait) {
		tv.tv_sec=0;tv.tv_usec=0;
		FD_ZERO(&rfd);FD_SET(sock,&rfd);
		rc=select(sock+1,&rfd,NULL,NULL,&tv);
		if(rc<1) {
			if(!rc)errno=EAGAIN;
			return -1;
		}
	}
	rc=recv(sock,&l,2,MSG_PEEK|MSG_WAITALL);
	if(rc<=0)return rc;
	if(rc==2) {
		l=I2H16(l);
		if(!l)return 0;
		if(l>len)l=len;
		rc=recv(sock,buf,MIN(l+2,len),MSG_WAITALL);
		if(rc<=0)return rc;
		rc=MIN(rc-2,FETCH16(buf));
		if(rc<1)return 0;
		if(rc>=len)rc=len-2;
		memcpy(buf,buf+2,rc);
		return rc;
	}
	return 0;
}
